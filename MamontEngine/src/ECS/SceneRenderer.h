#pragma once

#include <Core/Camera.h>

#include <Graphics/Vulkan/Pipelines/RenderPipeline.h>

namespace MamontEngine
{
    struct alignas(16) GPUSceneData
    {
        glm::mat4 View{glm::mat4(0.f)};
        glm::mat4 Proj{glm::mat4(0.f)};
        glm::mat4 Viewproj{glm::mat4(0.f)};
        glm::vec3 CameraPosition{glm::vec3(0.0)};
    };

    class Scene;

    class SceneRenderer
    {
    public:
        explicit SceneRenderer(const std::shared_ptr<Camera> &inCamera, const std::shared_ptr<Scene>& inScene);

        ~SceneRenderer();

        void Render(VkCommandBuffer inCmd, VkDescriptorSet globalDescriptor, const RenderPipeline *inRenderPipeline);

        void RenderPicking(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkPipeline inPipeline, VkPipelineLayout inLayout);

        void Update(const VkExtent2D &inWindowExtent, const std::array<Cascade, CASCADECOUNT> &inCascades, float inDeltaTime);

        GPUSceneData &GetGPUSceneData()
        {
            return m_SceneData;
        }
        const GPUSceneData &GetGPUSceneData() const
        {
            return m_SceneData;
        }

        const Camera *GetCamera() const
        {
            return m_Camera.get();
        }

        const DrawContext& GetDrawContext() const
        {
            return m_DrawContext;
        }

        void ClearDrawContext()
        {
            m_DrawContext.Clear();
        }

        const bool HasDirectionLight() const
        {
            return m_HasDirectionLight;
        }

        bool IsDrawCollisionBounds() const { return m_DrawCollisionBounds; }
        void EnableDrawCollisionBounds(bool value){ m_DrawCollisionBounds = value; }

        inline const LightData &GetLightData() const { return m_LightData; }

    private:
        std::shared_ptr<Scene>     m_Scene;
        std::shared_ptr<Camera>    m_Camera;
        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;
        LightData  m_LightData;

        bool m_HasDirectionLight{false};

        bool m_DrawCollisionBounds{false};
    };

} // namespace MamontEngine

