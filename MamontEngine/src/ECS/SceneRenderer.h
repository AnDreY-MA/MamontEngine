#pragma once

#include <Core/Camera.h>

#include <Graphics/Vulkan/Pipelines/RenderPipeline.h>

namespace MamontEngine
{
    struct GPUSceneData
    {
        glm::mat4 View{glm::mat4(0.f)};
        glm::mat4 Proj{glm::mat4(0.f)};
        glm::mat4 Viewproj{glm::mat4(0.f)};
        glm::vec3 LightDirection{glm::vec3(0.4f)};
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
        const CascadeData &GetCascadeData() const
        {
            return m_CascadeData;
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

    private:
        std::shared_ptr<Scene>     m_Scene;
        std::shared_ptr<Camera>    m_Camera;
        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;
        CascadeData  m_CascadeData;

        bool m_HasDirectionLight{false};
    };

} // namespace MamontEngine

