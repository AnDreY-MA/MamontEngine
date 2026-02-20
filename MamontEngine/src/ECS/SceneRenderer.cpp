#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>
#include "Core/Log.h"
#include "Graphics/Resources/Models/Model.h"
#include "Utils/Profile.h"
#include "Utils/VkImages.h"
#include "Utils/VkInitializers.h"
#include "Utils/VkPipelines.h"
#include "ECS/Scene.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "ECS/Components/TransformComponent.h"
#include <glm/gtx/quaternion.hpp>
#include "Graphics/DebugRenderer.h"
#include "Physics/Collision/BoxCollision.h"

namespace MamontEngine
{
    namespace
    {
        bool IsVisible(const AABB &bound, const glm::mat4 inTrasnform, const glm::mat4 &viewproj)
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

            const glm::mat4 matrix = viewproj * inTrasnform;

            glm::vec3 min = {1.5, 1.5, 1.5};
            glm::vec3 max = {-1.5, -1.5, -1.5};

            for (int c = 0; c < 8; c++)
            {
                glm::vec4 v = matrix * glm::vec4(bound.Center() + (corners[c] * bound.Extent()), 1.f);

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

        glm::vec3 GetForwardVector(const glm::quat inRotation)
        {
            const float yaw   = inRotation.y;
            const float pitch = inRotation.z;

            glm::vec3 forwardVector{glm::vec3(0)};
            forwardVector.x  = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
            forwardVector.y  = std::sin(glm::radians(pitch));
            forwardVector.z  = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));

            return forwardVector;
        }
    }

    SceneRenderer::SceneRenderer(const std::shared_ptr<Camera> &inCamera, const std::shared_ptr<Scene> &inScene) 
        : m_Camera(inCamera), m_Scene(inScene)
    {

    }

    SceneRenderer::~SceneRenderer()
    {
        m_DrawContext.Clear();
    }

    void SceneRenderer::Render(VkCommandBuffer     inCmd,
                               VkDescriptorSet     globalDescriptor,
                               const RenderPipeline* inRenderPipeline)
    {
        PROFILE_ZONE("SceneRenderer::Render");

        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inRenderPipeline->OpaquePipeline->Pipeline);

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inRenderPipeline->OpaquePipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

        constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};

        const auto draw = [&](const RenderObject &r)
        {
            vkCmdBindDescriptorSets(
                    inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inRenderPipeline->OpaquePipeline->Layout, 1, 1, &r.MaterialDescriptorSet, 0, nullptr);

            constexpr VkDeviceSize offsets[1] = {0};
            if (r.MeshBuffer.VertexBuffer.Buffer != VK_NULL_HANDLE)
            {
                vkCmdBindVertexBuffers(inCmd, 0, 1, &r.MeshBuffer.VertexBuffer.Buffer, offsets);
            }
            if (r.MeshBuffer.IndexBuffer.Buffer != VK_NULL_HANDLE)
            {
                vkCmdBindIndexBuffer(inCmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const GPUDrawPushConstants push_constants{
                    .WorldMatrix  = r.Transform, .VertexBuffer = r.MeshBuffer.VertexBufferAddress,
                    //.CascadeIndex = cascadeIndex
            };

            vkCmdPushConstants(inCmd, 
                inRenderPipeline->OpaquePipeline->Layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               constantsSize,
                               &push_constants);

            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);

            
        };

        // Opaque Draw
       for (const auto &object : m_DrawContext.OpaqueSurfaces)
       {
            if (IsVisible(object.Bound, object.Transform, m_SceneData.Viewproj))
            {
                draw(object);
            }
       }

       if (const auto vertexCount = DebugRenderer::GetVertexCount(); vertexCount > 0)
       {
           vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inRenderPipeline->DebugDrawPipeline->Pipeline);
           
           const auto                 bufferAdress = DebugRenderer::GetVertexBufferAdress();
           const GPUDrawPushConstants debug_pushconstants{
                   .WorldMatrix = glm::mat4(1.f), .VertexBuffer = bufferAdress,
           };

           vkCmdPushConstants(inCmd,
                              inRenderPipeline->DebugDrawPipeline->Layout,
                              VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                              0,
                              constantsSize,
                              &debug_pushconstants);
           vkCmdDraw(inCmd, vertexCount, 1, 0, 0);
       }

        /*for (const auto &r : transp_draws)
        {
            draw(m_DrawContext.TransparentSurfaces[r]);
        }*/

    }

    void SceneRenderer::RenderPicking(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkPipeline inPipeline, VkPipelineLayout inLayout)
    {
        uint64_t               idRenderObject{1};
        constexpr VkDeviceSize offsets[1] = {0};

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inPipeline);
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, inLayout, 0, 1, &globalDescriptor, 0, nullptr);

        const auto draw = [&](const RenderObject &r) -> void
        {
            vkCmdBindVertexBuffers(cmd, 0, 1, &r.MeshBuffer.VertexBuffer.Buffer, offsets);

            vkCmdBindIndexBuffer(cmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

            const GPUDrawPushConstants pushConstants{.WorldMatrix = r.Transform, .VertexBuffer = r.MeshBuffer.VertexBufferAddress, .ObjectID = (uint64_t)r.Id};

            constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};
            vkCmdPushConstants(cmd, inLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, constantsSize, &pushConstants);

            vkCmdDrawIndexed(cmd, r.IndexCount, 1, r.FirstIndex, 0, 0);

            ++idRenderObject;
        };

        for (const auto &object : m_DrawContext.OpaqueSurfaces)
        {
            draw(object);
        }
    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent, const std::array<Cascade, CASCADECOUNT> &inCascades, float inDeltaTime)
    {
        PROFILE_FUNCTION();

        ClearDrawContext();
        DebugRenderer::ClearVertex();

        const glm::mat4 &view = m_Camera->GetViewMatrix();

        m_Camera->UpdateProjection(inWindowExtent);
        const glm::mat4 &projection = m_Camera->GetProjection();

        const auto& sceneRegistry = m_Scene->GetRegistry();

        m_SceneData.View           = view;
        m_SceneData.Proj           = projection;
        m_SceneData.Viewproj       = projection * view;
        m_SceneData.CameraPosition = m_Camera->GetPosition();

        {
            const auto &viewDirectionLight = sceneRegistry.view<const DirectionLightComponent>();
            m_HasDirectionLight            = !viewDirectionLight.empty();
        }

        if (m_HasDirectionLight)
        {
            const auto &viewDirectionLight = sceneRegistry.view<const DirectionLightComponent, TransformComponent>();

            viewDirectionLight.each([&](const auto& ligth, const auto& transform) { 
                m_CascadeData.Color        = ligth.GetColor();
                const glm::quat rotation       = transform.Transform.Rotation;
                const glm::vec3 lightDirection = GetForwardVector(rotation);
                m_SceneData.LightDirection   = lightDirection;
                m_CascadeData.LightDirection = lightDirection;
            });
            for (size_t i = 0; i < CASCADECOUNT; i++)
            {
                m_CascadeData.Splits[i] = inCascades[i].SplitDepth;
            }

            m_CascadeData.InverseViewMatrix = glm::inverse(view);
        }
        else
        {
            m_CascadeData.Color = glm::vec3(0.3f, 0.3f, 0.3f);
            m_SceneData.LightDirection   = glm::vec3(.0f);
            m_CascadeData.LightDirection = glm::vec3(.0f);
        }

        m_CascadeData.IsActive = m_HasDirectionLight;

        const auto meshes = sceneRegistry.view<MeshComponent, TransformComponent>();
        for (const auto&& [entity, meshComponent, Transform] : meshes.each())
        {
            if (meshComponent.Mesh != nullptr)
            {
                meshComponent.Mesh->Draw(m_DrawContext);
            }
        }

        const auto viewCollisions = sceneRegistry.view<TransformComponent, HeroPhysics::BoxCollision>();
        for (auto [entity, transform, collision] : viewCollisions.each())
        {
            DebugRenderer::Draw(collision.GetBounds().Transform(transform.Matrix()), Color::GREEN);
        }

        DebugRenderer::DrawPoint(glm::vec3(1.f), 2, Color::WHITE );

        DebugRenderer::Update();

    }

} // namespace MamontEngine

