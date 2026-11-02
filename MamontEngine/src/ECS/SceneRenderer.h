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

        float NearPlane{0.01f};
        float FarPlane{10.f};
    };

    class Scene;
    class DirectLightPass;

    class SceneRenderer
    {
    public:
        explicit SceneRenderer(const std::shared_ptr<Camera> &inCamera, const std::shared_ptr<Scene>& inScene, const std::shared_ptr<DirectLightPass>& inDirectPass);

        ~SceneRenderer();

        void Render(VkCommandBuffer inCmd, std::pair<VkDescriptorSet, VkDescriptorSet> globalDescriptor, const RenderPipeline *inRenderPipeline);

        void Update(const VkExtent2D &inWindowExtent, float inDeltaTime);

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
            return m_LightData.HasDirectionLight;
        }

        bool IsDrawCollisionBounds() const { return m_DrawCollisionBounds; }
        void EnableDrawCollisionBounds(bool value){ m_DrawCollisionBounds = value; }

        inline const LightData &GetLightData() const { return m_LightData; }

        inline uint32_t GetDrawObjectCount() const { return m_DrawObjectCount; }

    private:
        std::shared_ptr<Scene>     m_Scene;
        std::shared_ptr<Camera>    m_Camera;

        std::weak_ptr<DirectLightPass> m_DirectShadowPass;

        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;
        LightData  m_LightData;

        uint32_t m_DrawObjectCount{0};

        bool m_DrawCollisionBounds{false};
    };

} // namespace MamontEngine

