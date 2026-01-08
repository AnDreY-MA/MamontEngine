#pragma once

#include "Camera.h"
#include "FrameData.h"
#include "Window.h"
#include "ContextDevice.h"
#include "Graphics/Renderer.h"
#include "ImGuiRenderer.h"
#include <ECS/SceneRenderer.h>
#include "ImGuiLayer.h"
#include "Core/Log.h"

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

        const TracyVkCtx& GetTracyContext() const
        {
            return m_ContextDevice->GetCurrentFrame().TracyContext;
        }

        void EnableCascade(bool inValue)
        {
            m_Renderer->EnableCascade(inValue);
        }

        VkContextDevice& GetContextDevice()
        {
            return *m_ContextDevice;
        }
        const VkContextDevice &GetContextDevice() const 
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

        std::shared_ptr<SceneRenderer>& GetSceneRenderer()
        {
            return m_Renderer->GetSceneRenderer();
        }
        

        const WindowCore* GetMainWindow() const
        {
            return m_Window.get();
        }

        void PushGuiLayer(ImGuiLayer *inLayer);

        const SDL_Event* GetInputEvent() const
        {
            return m_InputEvent;
        }

        uint64_t TryPickObject(const glm::vec2 &inMousePos)
        {
            return m_Renderer->TryPickObject(inMousePos);
        }

    private:
        void InitPipelines();

        void InitImgui();

        void InitScene();
        void UpdateScene(float inDeltaTime);

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