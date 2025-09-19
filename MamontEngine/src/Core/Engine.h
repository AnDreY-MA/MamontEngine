#pragma once

#include "VkDestriptor.h"
#include "Utils/Loader.h"
#include "Camera.h"
#include "FrameData.h"
#include "Window.h"
#include "ContextDevice.h"
#include "Graphics/Vulkan/MeshBuffer.h"
#include "Graphics/Renderer.h"
#include "ImGuiRenderer.h"
#include <ECS/SceneRenderer.h>
#include "ImGuiLayer.h"
#include "Core/Log.h"

struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    class Scene;
    
	class MEngine
	{
    public:
        void Init();
        void Run();
        void Cleanup();

        static MEngine& Get();

        VkContextDevice& GetContextDevice()
        {
            return *m_ContextDevice;
        }

        const RenderStats& GetStats() const
        {
            return m_Renderer->GetStats();
        }

        std::shared_ptr<Scene>& GetScene()
        {
            return m_Scene;
        }

        void PushGuiLayer(ImGuiLayer *inLayer);

        const SDL_Event* GetInputEvent() const
        {
            return m_InputEvent;
        }

    private:
        void InitDefaultData();

        void InitPipelines();

        void InitImgui();

        void ResizeSwapchain();

        void InitScene();
        void UpdateScene();

        VkFormat FindDepthFormat() const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    private:
        bool       m_IsInitialized{false};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        std::shared_ptr<WindowCore> m_Window;

        std::unique_ptr<Renderer>      m_Renderer;
        std::unique_ptr<ImGuiLayer>    m_GuiLayer;
        std::unique_ptr<Log>           m_Log;

        std::unique_ptr<VkContextDevice> m_ContextDevice;

        DeletionQueue m_MainDeletionQueue;

        std::shared_ptr<Scene> m_Scene;

        SDL_Event* m_InputEvent;

        std::shared_ptr<Camera> m_MainCamera;
	
    };
}