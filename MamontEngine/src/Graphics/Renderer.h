#pragma once

#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "ECS/SceneRenderer.h"

namespace MamontEngine
{
    struct VkContextDevice;
    class WindowCore;
    class ImGuiRenderer;
    struct DrawContext;
    class Camera;
    class Scene;
    class MeshModel;
    class DirectLightPass;

    struct RenderStats
    {
        float FrameTime{0.f};
        float SceneUpdateTime{0.f};
        float MeshDrawTime{0.f};
        int   TriangleCount{0};
        int   DrawCallCount{0};
    };

    class Renderer : public NonMovable
	{
    public:
        Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore>& inWindow);
        ~Renderer();

        void InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera, const std::shared_ptr<Scene> &inScene);
        void InitImGuiRenderer();

        void InitPipelines();
        void DestroyPipelines();

        void Render();

        void DrawPickingPass(VkCommandBuffer cmd, const uint32_t inCurrentSwapchainIndex);
        uint64_t TryPickObject(const glm::vec2 &inMousePos);

        void UpdateSceneRenderer(float inDeltaTime);

        inline const RenderStats& GetStats() const
        {
            return m_Stats;
        }


        std::shared_ptr<SceneRenderer>& GetSceneRenderer()
        {
            return m_SceneRenderer;
        }

        void EnableCascade(bool inValue);

    private:
        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);
        void DrawSkybox(VkCommandBuffer inCmd);

        void RenderCascadeShadow(VkCommandBuffer inCmd);

        void SetViewportScissor(VkCommandBuffer cmd, const VkExtent2D &inExtent) const;

        void UpdateUniformBuffers();

        void InitPickPipepline();

	private:
        VkContextDevice &m_DeviceContext;

        std::shared_ptr<SceneRenderer> m_SceneRenderer;

        std::unique_ptr<ImGuiRenderer> m_ImGuiRenderer;

        std::shared_ptr<WindowCore> m_Window;

        std::shared_ptr<RenderPipeline> m_RenderPipeline;
        std::unique_ptr<PipelineData> m_PickingPipeline;

        std::unique_ptr<DirectLightPass> m_DirectLightPass;

        std::unique_ptr<MeshModel> m_Skybox;

        bool IsActiveCascade{true};

        VkExtent2D m_DrawExtent;
        float      m_RenderScale{1.f};

        RenderStats m_Stats;

        VkImage m_CurrentSwapchainImage;
        VkImageView m_CurrentSwapchainImageView;
	};
}