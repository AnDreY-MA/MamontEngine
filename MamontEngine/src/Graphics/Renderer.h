#pragma once

#include <SDL3/SDL_events.h>
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
        void InitImGuiRenderer(VkDescriptorPool &outDescPool);

        void InitPipelines();
        void DestroyPipelines();

        void Render();

        void UpdateWindowEvent(const SDL_EventType inType);

        void UpdateSceneRenderer();

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

    private:
        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

	private:
        VkContextDevice &m_DeviceContext;

        std::unique_ptr<RenderPipeline> m_RenderPipeline;

        std::unique_ptr<SkyPipeline> m_SkyPipeline;

        std::shared_ptr<SceneRenderer> m_SceneRenderer;

        std::unique_ptr<ImGuiRenderer> m_ImGuiRenderer;

        std::shared_ptr<WindowCore> m_Window;

        bool m_StopRendering{false};
        bool m_IsResizeRequested{false};

        VkExtent2D m_DrawExtent;
        float      m_RenderScale{1.f};

        RenderStats m_Stats;
	};
}