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

        void EnableCascade(bool inValue)
        {
            IsActiveCascade = inValue;
        }

    private:
        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

        void CreateShadowPipeline();

        void SetViewportScissor(VkCommandBuffer cmd, const VkExtent2D &inExtent) const;

        void UpdateUniformBuffers();

        void CalculateShadowData(const glm::mat4& inCameraView, const glm::vec4 inLightDirection);

        void CalculateLightSpaceMatrices(std::span<float> inCascadeLevels, const glm::vec4 inLightDirection, float zMult);

	private:
        VkContextDevice &m_DeviceContext;

        std::unique_ptr<RenderPipeline> m_RenderPipeline;

        std::unique_ptr<SkyPipeline> m_SkyPipeline;

        std::shared_ptr<SceneRenderer> m_SceneRenderer;

        std::unique_ptr<ImGuiRenderer> m_ImGuiRenderer;

        std::shared_ptr<WindowCore> m_Window;

        std::unique_ptr<PipelineData> m_CascadePipeline;

        bool IsActiveCascade{false};

        struct Settings
        {
            float Z_Near{0.1f};
            float Z_Far{100.f};
            
            struct
            {
                float DepthBiasConstantFactor{1.25f};
                float DetphBiasSlopeFactor{1.75f};
                float CascadeSplitLambda{0.95f};
                float ZMult{2.2f};
            } Shadow;

        } m_Settings;

        ShadowCascadeUBO m_ShadowCascadeUBOData;
        ShadowCascadeMatrices m_ShadowMatrices;

        bool m_StopRendering{false};
        bool m_IsResizeRequested{false};

        VkExtent2D m_DrawExtent;
        float      m_RenderScale{1.f};

        RenderStats m_Stats;
	};
}