#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>
#include "Graphics/Model.h"
#include "Utils/Profile.h"
#include "Core/Log.h"

namespace MamontEngine
{
    constexpr float FOV = 70.f;
    
    static bool     is_visible(const RenderObject &obj, const glm::mat4 &viewproj)
    {
        constexpr std::array<glm::vec3, 8> corners{
                glm::vec3{1, 1, 1},
                glm::vec3{1, 1, -1},
                glm::vec3{1, -1, 1},
                glm::vec3{1, -1, -1},
                glm::vec3{-1, 1, 1},
                glm::vec3{-1, 1, -1},
                glm::vec3{-1, -1, 1},
                glm::vec3{-1, -1, -1},
        };

        const glm::mat4 matrix = viewproj * obj.Transform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 v = matrix * glm::vec4(obj.Bound.Origin + (corners[c] * obj.Bound.Extents), 1.f);

            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
        {
            return false;
        }

        return true;
    }

    SceneRenderer::SceneRenderer(const std::shared_ptr<Camera> &inCamera) 
        : m_Camera(inCamera)
    {

    }
    
    SceneRenderer::~SceneRenderer() 
    {
        Clear();
    }

    void SceneRenderer::Clear()
    {
        m_MeshComponents.clear();
    }

    void SceneRenderer::Render(VkCommandBuffer inCmd, VkDescriptorSet globalDescriptor, const GPUSceneData &inSceneData)
    {
        PROFILE_ZONE("SceneRenderer::Render");
        std::vector<uint32_t> opaque_draws;
        opaque_draws.reserve(m_DrawContext.OpaqueSurfaces.size());

        for (size_t i = 0; i < m_DrawContext.OpaqueSurfaces.size(); i++)
        {
            if (is_visible(m_DrawContext.OpaqueSurfaces[i], inSceneData.Viewproj))
            {
                opaque_draws.push_back(i);
            }
        }

        std::vector<uint32_t> transp_draws;
        transp_draws.reserve(m_DrawContext.TransparentSurfaces.size());

        for (size_t i = 0; i < m_DrawContext.TransparentSurfaces.size(); i++)
        {
            if (is_visible(m_DrawContext.TransparentSurfaces[i], inSceneData.Viewproj))
            {
                transp_draws.push_back(i);
            }
        }

        std::shared_ptr<PipelineData> lastPipeline     = nullptr;
        const GLTFMaterial       *lastMaterial     = nullptr;
        VkBuffer                      lastIndexBuffer  = VK_NULL_HANDLE;
        VkBuffer                      lastVertexBuffer = VK_NULL_HANDLE;

        const auto draw = [&](const RenderObject &r)
        {
            if (r.Material != lastMaterial)
            {
                lastMaterial = r.Material;
                if (r.Material->Pipeline != lastPipeline)
                {
                    lastPipeline = r.Material->Pipeline;
                    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Pipeline);

                    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);
                }

                vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);
            }
            if (r.VertexBuffer != lastVertexBuffer)
            {
                constexpr VkDeviceSize offsets[1] = {0};
                lastVertexBuffer                  = r.VertexBuffer;
                vkCmdBindVertexBuffers(inCmd, 0, 1, &r.VertexBuffer, offsets);
            }

            if (r.IndexBuffer != lastIndexBuffer)
            {
                lastIndexBuffer = r.IndexBuffer;
                vkCmdBindIndexBuffer(inCmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const GPUDrawPushConstants push_constants{r.Transform, r.VertexBufferAddress};

            vkCmdPushConstants(inCmd, r.Material->Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };


        for (const auto &r : opaque_draws)
        {
            draw(m_DrawContext.OpaqueSurfaces[r]);
        }

        for (const auto &r : transp_draws)
        {
            draw(m_DrawContext.TransparentSurfaces[r]);
        }

        m_DrawContext.Clear();
    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent)
    {
        const glm::mat4& view       = m_Camera->GetViewMatrix();
        const glm::mat4& projection = m_Camera->GetProjection();

        m_Camera->UpdateProjection(inWindowExtent);

        m_SceneData.View     = view;
        m_SceneData.Proj     = projection;
        m_SceneData.Viewproj = projection * view;

        for (const auto &mesh : m_MeshComponents)
        {
            if (mesh.Mesh != nullptr)
            {
                mesh.Mesh->Draw(m_DrawContext);
            }
        }
    }

    void SceneRenderer::SubmitMesh(const MeshComponent& inMesh)
    {
        m_MeshComponents.push_back(inMesh);
    }
}