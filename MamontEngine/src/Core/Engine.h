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
#include "Graphics/Resources/Materials/MaterialManager.h"
#include "InputSystem.h"
#include "Audio/AudioSystem.h"

namespace MamontEngine
{
    class Scene;
    class TimeStep;

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

        const MemoryStats &GetGPUMemoryStats() const
        {
            return m_Renderer->GetGPUMemoryStats();
        }

        std::shared_ptr<Scene>& GetScene()
        {
            return m_Scene;
        }

        std::shared_ptr<SceneRenderer>& GetSceneRenderer() const
        {
            return m_Renderer->GetSceneRenderer();
        }

        Renderer* GetRenderer() const
        {
            return m_Renderer.get();
        }

        const WindowCore* GetMainWindow() const
        {
            return m_Window.get();
        }

        inline HeroPhysics::PhysicsSystem* GetPhysicsSytem() const { return m_PhysicsSystem.get(); }

        inline ScriptSystem *GetScriptSystem() const { return m_ScriptSystem.get(); }

        inline AudioSystem &GetAudioSystem() const { return *m_AudioSystem.get(); }

        void PushGuiLayer(ImGuiLayer *inLayer);

        const SDL_Event* GetInputEvent() const
        {
            return m_InputEvent;
        }

        struct Stats
        {
            double FrameTime{0.0};
        };

        inline const Stats &GetEngineStats() const { return m_Stats; }

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
        std::unique_ptr<MaterialManager> m_MaterialManager;
        std::unique_ptr<InputSystem>     m_InputSystem;

        std::unique_ptr<AudioSystem> m_AudioSystem;

        std::unique_ptr<HeroPhysics::PhysicsSystem> m_PhysicsSystem;

        std::unique_ptr<VkContextDevice> m_ContextDevice;

        DeletionQueue m_MainDeletionQueue;

        std::shared_ptr<Scene> m_Scene;

        SDL_Event* m_InputEvent;

        std::shared_ptr<Camera> m_MainCamera;

        std::chrono::microseconds m_DeltaTime{0};
        uint64_t                  m_LastFrameTime{0};

        Stats m_Stats;

        TimeStep* m_TimeStep;
    };
}
