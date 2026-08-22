#pragma once

#include "Camera.h"
#include "FrameData.h"
#include "Window.h"
#include "ContextDevice.h"
#include "Graphics/Renderer.h"
#include "Graphics/ImGuiRenderer.h"
#include <ECS/SceneRenderer.h>
#include "ImGuiLayer.h"
#include "Core/Log.h"
#include "Scripting/ScriptSystem.h"
#include "Physics/PhysicsSystem.h"

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

        inline HeroPhysics::PhysicsSystem* GetPhysicsSytem() const { return m_PhysicsSystem.get(); }

        inline ScriptSystem *GetScriptSystem() const { return m_ScriptSystem.get(); }

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
        void InitImgui();
        void UpdateScene(float inDeltaTime);

        std::chrono::milliseconds CalculateDeltaTimeMs();

    private:
        bool       m_IsInitialized{false};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        std::shared_ptr<WindowCore> m_Window;

        std::unique_ptr<Renderer>      m_Renderer;
        std::unique_ptr<ImGuiLayer>    m_GuiLayer;
        std::unique_ptr<Log>           m_Log;
        std::unique_ptr<ScriptSystem>  m_ScriptSystem;

        std::unique_ptr<HeroPhysics::PhysicsSystem> m_PhysicsSystem;

        std::unique_ptr<VkContextDevice> m_ContextDevice;

        DeletionQueue m_MainDeletionQueue;

        std::shared_ptr<Scene> m_Scene;

        SDL_Event* m_InputEvent;

        std::shared_ptr<Camera> m_MainCamera;

        std::chrono::microseconds m_DeltaTime{0};
        uint64_t                  m_LastFrameTime{0};
    };
}
