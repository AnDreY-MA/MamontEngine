#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>
#include "Graphics/Model.h"
#include "Utils/Profile.h"
#include "Core/Log.h"
#include "Core/VkInitializers.h"
#include "Core/VkImages.h"
#include "Core/VkPipelines.h"

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
            glm::vec4 v = matrix * glm::vec4(obj.Bound.Center() + (corners[c] * obj.Bound.Extent()), 1.f);

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
        constexpr float angle  = glm::radians(360.0f);
        constexpr float  radius = 20.0f;
        //m_LightPosition     = glm::vec3(cos(angle) * radius, -radius, sin(angle) * radius);
        m_LightPosition = glm::vec3(0.8f, 5.f, 34.f);
    }
    
    SceneRenderer::~SceneRenderer() 
    {
        Clear();
    }

    void SceneRenderer::Clear()
    {
        m_MeshComponents.clear();
    }

    void SceneRenderer::Render(VkCommandBuffer inCmd, VkDescriptorSet globalDescriptor, const glm::mat4 &inViewProjection, VkPipelineLayout inGlobalLayout, uint32_t cascadeIndex)
    {
        PROFILE_ZONE("SceneRenderer::Render");
        std::vector<uint32_t> opaque_draws;
        opaque_draws.reserve(m_DrawContext.OpaqueSurfaces.size());

        for (size_t i = 0; i < m_DrawContext.OpaqueSurfaces.size(); i++)
        {
            if (is_visible(m_DrawContext.OpaqueSurfaces[i], inViewProjection))
            {
                opaque_draws.push_back(i);
            }
        }

        std::vector<uint32_t> transp_draws;
        transp_draws.reserve(m_DrawContext.TransparentSurfaces.size());

        for (size_t i = 0; i < m_DrawContext.TransparentSurfaces.size(); i++)
        {
            if (is_visible(m_DrawContext.TransparentSurfaces[i], inViewProjection))
            {
                transp_draws.push_back(i);
            }
        }

        PipelineData*        lastPipeline = nullptr;
        const GLTFMaterial       *lastMaterial     = nullptr;

        const auto draw = [&](const RenderObject &r)
        {
            if (r.Material != lastMaterial)
            {
                lastMaterial = r.Material;
                if (r.Material->Pipeline.get() != lastPipeline)
                {
                    lastPipeline = r.Material->Pipeline.get();
                    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Pipeline);

                    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);
                }

                if (r.Material->MaterialSet != VK_NULL_HANDLE && r.Material->Pipeline->Layout != VK_NULL_HANDLE)
                {
                    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);
                }
            }

            constexpr VkDeviceSize offsets[1] = {0};
            if (r.VertexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindVertexBuffers(inCmd, 0, 1, &r.VertexBuffer, offsets);
            }
            if (r.IndexBuffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(inCmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const GPUDrawPushConstants push_constants{
                .WorldMatrix = r.Transform, 
                .VertexBuffer = r.VertexBufferAddress, 
                //.CascadeIndex = cascadeIndex
            };

            constexpr uint32_t         constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
            vkCmdPushConstants(
                    inCmd, r.Material->Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &push_constants);

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

        //m_DrawContext.Clear();
    }

    void SceneRenderer::RenderShadow(VkCommandBuffer            inCmd,
                                     VkDescriptorSet            globalDescriptor,
                                     const glm::mat4           &inViewProjection,
                                     const PipelineData &inPipelineData,
                                     uint32_t                   cascadeIndex)
    {
        PROFILE_ZONE("SceneRenderer::RenderShadow");
        std::vector<uint32_t> opaque_draws;
        opaque_draws.reserve(m_DrawContext.OpaqueSurfaces.size());

        for (size_t i = 0; i < m_DrawContext.OpaqueSurfaces.size(); i++)
        {
            if (is_visible(m_DrawContext.OpaqueSurfaces[i], inViewProjection))
            {
                opaque_draws.push_back(i);
            }
        }

        const auto draw = [&](const RenderObject &r)
        {
            vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipelineData.Pipeline);

            vkCmdBindDescriptorSets(inCmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS, inPipelineData.Layout,
                                    0,
                                    1,
                                    &globalDescriptor,
                                    0,
                                    nullptr);
            //vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);

            constexpr VkDeviceSize offsets[1] = {0};
            vkCmdBindVertexBuffers(inCmd, 0, 1, &r.VertexBuffer, offsets);

            vkCmdBindIndexBuffer(inCmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

            const GPUDrawPushConstants push_constants {
                .WorldMatrix = r.Transform, 
                .VertexBuffer = r.VertexBufferAddress, 
                .CascadeIndex = cascadeIndex
            };

            constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
            vkCmdPushConstants(inCmd, inPipelineData.Layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0, constantsSize,
                               &push_constants);

            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);

        };

        for (const auto &r : opaque_draws)
        {
            draw(m_DrawContext.OpaqueSurfaces[r]);
        }
    }

    void SceneRenderer::RenderPicking(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkPipeline inPipeline, VkPipelineLayout inLayout)
    {
        uint64_t   idRenderObject{1};
        constexpr VkDeviceSize offsets[1] = {0};

        const auto draw = [&](const RenderObject& r) -> void
        {
            vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline);
            //vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inLayout, 0, 1, &globalDescriptor, 0, nullptr);

            vkCmdBindVertexBuffers(cmd, 0, 1, &r.VertexBuffer, offsets);

            vkCmdBindIndexBuffer(cmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

            const GPUDrawPushConstants pushConstants{
                    .WorldMatrix = r.Transform, 
                    .VertexBuffer = r.VertexBufferAddress, 
                    .ObjectID = (uint64_t)r.Id
            };

            constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
            vkCmdPushConstants(cmd, inLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &pushConstants);

            vkCmdDrawIndexed(cmd, r.IndexCount, 1, r.FirstIndex, 0, 0);

            ++idRenderObject;
        };

        for (const auto& object : m_DrawContext.OpaqueSurfaces)
        {
            draw(object);
        }
    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent, const std::array<Cascade, CASCADECOUNT> &inCascades)
    {
        const glm::mat4& view       = m_Camera->GetViewMatrix();
        const glm::mat4& projection = m_Camera->GetProjection();
        const auto       lightDirection{glm::normalize(-m_LightPosition)};

        m_Camera->UpdateProjection(inWindowExtent);
        m_SceneData.View     = view;
        m_SceneData.Proj     = projection;
        m_SceneData.Viewproj = projection * view;
        m_SceneData.LightDirection = lightDirection;

        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            m_CascadeData.Splits[i] = inCascades[i].SplitDepth;
        }

        m_CascadeData.InverseViewMatrix  = glm::inverse(view);
        m_CascadeData.LightDirection = lightDirection;
        m_CascadeData.Color   = 0;

        UpdateLight();

        for (const auto &mesh : m_MeshComponents)
        {
            if (mesh.Mesh != nullptr)
            {
                mesh.Mesh->Draw(m_DrawContext);
            }
        }
    }

    void SceneRenderer::UpdateLight()
    {
        const float angle = glm::radians(0.2f * 360.f);
        const float radius{20.f};

        m_LightPosition = glm::vec3(cos(angle) * radius, -radius, sin(angle) * radius);
    }

    void SceneRenderer::UpdateCascades(std::array<Cascade, CASCADECOUNT> &outCascades)
    {
        std::array<float, CASCADECOUNT> cascadeSplits{};

        const float nearClip{m_Camera->GetNearClip()};
        const float farClip{m_Camera->GetFarClip()};
        const float clipRange{farClip - nearClip};
        
        const float minZ = nearClip;
        const float maxZ = nearClip + clipRange;

        const float range = maxZ - minZ;
        const float ratio = maxZ / minZ;

        constexpr float cascadeSplitLambda{0.95f};

        for (size_t i{ 0 }; i < CASCADECOUNT; i++)
        {
            const float p = static_cast<float>((i + 1) / CASCADECOUNT);
            const float log = minZ * std::pow(ratio, p);
            const float uniform = minZ + range * p;
            const float d       = cascadeSplitLambda * (log - uniform) + uniform;
            cascadeSplits[i]    = (d - nearClip) / clipRange;
        }

        float lastSplitDist = 0;
        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            float splitDist = cascadeSplits[i];
            glm::vec3 frustumCorners[8] = {
                    glm::vec3(-1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, 1.0f, 0.0f),
                    glm::vec3(1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, -1.0f, 0.0f),
                    glm::vec3(-1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, 1.0f, 1.0f),
                    glm::vec3(1.0f, -1.0f, 1.0f),
                    glm::vec3(-1.0f, -1.0f, 1.0f),
            };

            const glm::mat4 inverseCamera = glm::inverse(m_Camera->GetViewMatrix() * m_Camera->GetProjection());
            for (uint32_t j = 0; j < 8; j++)
            {
                const glm::vec4 invCorner = inverseCamera * glm::vec4(frustumCorners[j], 1.f);
                frustumCorners[j]         = invCorner / invCorner.w;
            }

            for (uint32_t j = 0; j < 4; j++)
            {
                const glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
                frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
                frustumCorners[j] = frustumCorners[j] + (dist * splitDist);
            }

            glm::vec3 frustumCenter = glm::vec3(0.f);
            for (uint32_t j = 0; j < 8; j++)
            {
                frustumCenter += frustumCorners[j];
            }
            frustumCenter /= 8.f;

            float radius = 0.f;
            for (uint32_t j = 0; j < 8; j++)
            {
                const float distance = glm::length(frustumCorners[j] - frustumCenter);
                radius               = glm::max(radius, distance);
            }
            radius = std::ceil(radius * 16.f) / 16.f;

            const glm::vec3 maxExtents = glm::vec3(radius);
            const glm::vec3 minExtents = -maxExtents;

            const glm::vec3 lightDirection = glm::normalize(-m_LightPosition);
            const glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDirection * -minExtents.z, frustumCenter, glm::vec3(0.f, 1.f, 0.f));
            const glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.f, maxExtents.z - minExtents.z);

            outCascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.f;
            outCascades[i].ViewProjectMatrix = lightOrthoMatrix * lightViewMatrix;

            lastSplitDist = cascadeSplits[i];
        }
    }

    void SceneRenderer::SubmitMesh(const MeshComponent& inMesh)
    {
        m_MeshComponents.push_back(inMesh);
    }
}