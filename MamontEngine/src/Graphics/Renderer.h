#pragma once

#include "Graphics/Vulkan/Pipelines/SkyPipeline.h"
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "ECS/SceneRenderer.h"

namespace MamontEngine
{
    struct VkContextDevice;
    class WindowCore;
    class ImGuiRenderer;
    struct DrawContext;
    class Camera;

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
        ~Renderer() = default;

        void InitSceneRenderer(const std::shared_ptr<Camera>& inMainCamera);
        void InitImGuiRenderer();

        void InitPipelines();
        void DestroyPipelines();

        void Render();

        void DrawPickingPass(VkCommandBuffer cmd, const uint32_t inCurrentSwapchainIndex);
        uint64_t TryPickObject(const glm::vec2 &inMousePos);

        void UpdateWindowEvent(const SDL_EventType inType);

        void UpdateSceneRenderer(float inDeltaTime);

        void ResizeSwapchain();

        void Clear();

        inline bool IsStopRendering() const
        {
            return m_StopRendering;
        }

        inline bool IsResizeRequested() const
        {
            return m_IsResizeRequested;
        }

        inline const RenderStats& GetStats() const
        {
            return m_Stats;
        }

        DrawContext &GetDrawContext();

        const std::shared_ptr<SceneRenderer>& GetSceneRenderer() const
        {
            return m_SceneRenderer;
        }

        void EnableCascade(bool inValue);

    private:
        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

        void RenderCascadeShadow(VkCommandBuffer inCmd);

        void CreateShadowPipeline();

        void SetViewportScissor(VkCommandBuffer cmd, const VkExtent2D &inExtent) const;

        void UpdateUniformBuffers();

        void InitPickPipepline();

	private:
        VkContextDevice &m_DeviceContext;

        std::unique_ptr<RenderPipeline> m_RenderPipeline;

        std::unique_ptr<SkyPipeline> m_SkyPipeline;

        std::shared_ptr<SceneRenderer> m_SceneRenderer;

        std::unique_ptr<ImGuiRenderer> m_ImGuiRenderer;

        std::shared_ptr<WindowCore> m_Window;

        std::unique_ptr<PipelineData> m_CascadePipeline;
        std::unique_ptr<PipelineData> m_PickingPipeline;

        bool IsActiveCascade{true};

        bool m_StopRendering{false};
        bool m_IsResizeRequested{false};

        VkExtent2D m_DrawExtent;
        float      m_RenderScale{1.f};

        RenderStats m_Stats;

        VkImage m_CurrentSwapchainImage;
        VkImageView m_CurrentSwapchainImageView;
	};
}