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
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/TransformComponent.h"
#include <glm/gtx/quaternion.hpp>
#include "Graphics/DebugRenderer.h"
#include "Physics/Collision/BoxCollision.h"
#include "Physics/Collision/SphereCollision.h"
#include "Graphics/Resources/Materials/MaterialAllocator.h"
#include "Graphics/Frustum.h"

namespace
{
    std::vector<glm::vec3> GetBoxVertices(const MamontEngine::AABB &aabb)
    {
        std::vector<glm::vec3> vertices;
        vertices.resize(8);

        // Lower
        vertices[0].x = aabb.Min.x;
        vertices[0].y = aabb.Min.y;
        vertices[0].z = aabb.Min.z;

        vertices[1].x = aabb.Min.x;
        vertices[1].z = aabb.Min.z;

        vertices[2].z = aabb.Min.z;

        vertices[3].y = aabb.Min.y;
        vertices[3].z = aabb.Min.z;

        vertices[4].x = aabb.Min.x;
        vertices[4].y = aabb.Min.y;

        vertices[5].x = aabb.Min.x;

        vertices[7].y = aabb.Min.y;

        // Upper
        vertices[1].y = aabb.Max.y;

        vertices[2].x = aabb.Max.x;
        vertices[2].y = aabb.Max.y;

        vertices[3].x = aabb.Max.x;

        vertices[4].z = aabb.Max.z;

        vertices[5].y = aabb.Max.y;
        vertices[5].z = aabb.Max.z;

        vertices[6].x = aabb.Max.x;
        vertices[6].y = aabb.Max.y;
        vertices[6].z = aabb.Max.z;

        vertices[7].x = aabb.Max.x;
        vertices[7].z = aabb.Max.z;

        return vertices;
    }
} // namespace

namespace MamontEngine
{
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
            
            vkCmdBindIndexBuffer(inCmd, r.MeshBuffer.IndexBuffer.Buffer, 0, VK_INDEX_TYPE_UINT32);

            const GPUDrawPushConstants push_constants
            {
                .WorldMatrix = r.Transform, 
                .VertexBuffer = r.MeshBuffer.VertexBuffer.Address, 
                .MaterialBuffer = MaterialAllocator::GetBufferAddess(),
                .MaterialIndex = r.MaterialIndex
            };

            vkCmdPushConstants(inCmd, 
                inRenderPipeline->OpaquePipeline->Layout,
                               VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                               0,
                               constantsSize,
                               &push_constants);

            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };

        uint32_t drawObjectCount{0};
        const auto frustumPlanes = ExtractFrustumPlanes(m_SceneData.Viewproj);
        // Opaque Draw
        for (const auto &object : m_DrawContext.OpaqueSurfaces)
        {
            if (IsNodeVisible(frustumPlanes, object.Transform, object.SphereBound))
            {
                draw(object);
            }

            drawObjectCount++;
        }

       // Log::Info("Draw ObjectCount: {}", drawObjectCount);

       /* const auto& meshView = m_Scene->GetRegistry().view<MeshComponent>();
        for (const auto& [entity, mesh] : meshView.each())
        {
            const auto& model = mesh.Mesh;
            if (!model)
                continue;


        }
        const glm::mat4 viewProj         = m_Camera->GetProjection() * m_Camera->GetViewMatrix();
        const FrustumPlane    frustumPlane = ExtractFrustumPlanes(viewProj);*/

        if (m_DrawCollisionBounds)
        {
            DebugRenderer::Render(inCmd);
        }

        /*for (const auto &r : transp_draws)
        {
            draw(m_DrawContext.TransparentSurfaces[r]);
        }*/

    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent, float inDeltaTime)
    {
        PROFILE_FUNCTION();

        ClearDrawContext();
        DebugRenderer::ClearVertex();

        m_Camera->UpdateProjection(inWindowExtent);

        const glm::mat4 &view = m_Camera->GetViewMatrix();
        const glm::mat4 &projection = m_Camera->GetProjection();

        const auto& sceneRegistry = m_Scene->GetRegistry();

        m_SceneData.View           = view;
        m_SceneData.Proj           = projection;
        m_SceneData.Viewproj       = projection * view;
        m_SceneData.CameraPosition = m_Camera->GetPosition();
        m_SceneData.NearPlane      = m_Camera->GetNearClip();
        m_SceneData.FarPlane = m_Camera->GetFarClip();

        {
            const auto &viewDirectionLight = sceneRegistry.view<const DirectionLightComponent>();
            m_HasDirectionLight            = !viewDirectionLight.empty();
        }

        {
            const auto &viewPointLight = sceneRegistry.view<const PointLightComponent>();
            m_LightData.PointLightingCount                = viewPointLight.size();
        }

        if (m_HasDirectionLight)
        {
            const auto &viewDirectionLight = sceneRegistry.view<const DirectionLightComponent, TransformComponent>();

            viewDirectionLight.each([&](const auto& ligth, const auto& transform) { 
                m_LightData.DirectLight.Direction = transform.GetForwardVector();
                m_LightData.DirectLight.Color     = ligth.GetColor();

            });
        }
        
        else
        {
            m_LightData.DirectLight.Color          = glm::vec3(0.3f, 0.3f, 0.3f);
            m_LightData.DirectLight.Direction = glm::vec3(.0f);
        }

        if (m_LightData.PointLightingCount > 0)
        {
            const auto &viewPointLight = sceneRegistry.view<const PointLightComponent, TransformComponent>();

            uint32_t indexLight = 0;
            viewPointLight.each(
                    [&](const PointLightComponent &ligthComponent, const TransformComponent &transform)
                    {
                        auto &light                           = m_LightData.PointLights[indexLight];
                        light.Color                           = ligthComponent.GetColor();
                        light.Radius                          = ligthComponent.GetRadius();
                        light.Position                        = transform.GetPosition();
                        light.Attenuation                     = ligthComponent.GetAttenuation();
                        ++indexLight;
                    });

        }

        m_LightData.HasDirectionLight = m_HasDirectionLight;

        const auto meshes = sceneRegistry.view<MeshComponent, TransformComponent>();
        for (const auto&& [entity, meshComponent, Transform] : meshes.each())
        {
            if (meshComponent.Mesh != nullptr)
            {
                meshComponent.Mesh->Draw(m_DrawContext);
            }
        }

        static bool stopPlane = false;
        static glm::mat4   invViewProj = glm::mat4(1.f);
        if (m_DrawCollisionBounds)
        {
           /* const auto viewCollisions = sceneRegistry.view<TransformComponent, HeroPhysics::BoxCollision>();
            for (auto [entity, transform, collision] : viewCollisions.each())
            {
                DebugRenderer::Draw(collision.GetBounds().Transform(transform.Matrix()), Color::GREEN);
            }

            const auto viewSphereCollisions = sceneRegistry.view<TransformComponent, HeroPhysics::SphereCollision>();
            for (auto [entity, transform, collision] : viewSphereCollisions.each())
            {
                DebugRenderer::Draw(collision.GetBounds().Transform(transform.Matrix()), Color::BLUE);
            }*/

            

             
            /*if (stopPlane == false)
            {
                invViewProj = glm::inverse(m_SceneData.Viewproj);
                stopPlane   = true;
            }

            DebugRenderer::DrawFrustum(invViewProj);*/

            //DebugRenderer::DrawLine(glm::vec3(1), glm::vec3(5));

            DebugRenderer::Update();

        }
    }

} // namespace MamontEngine