#include "Engine.h"
#include <thread>

#include "Graphics/Vulkan/Swapchain.h"

#include "ECS/Entity.h"

#include "ECS/Components/TransformComponent.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Utils/Profile.h"
#include "tracy/public/TracyClient.cpp"
#include "Graphics/Resources/Materials/MaterialAllocator.h"
#include "Core/JobSystem.h"

namespace MamontEngine
{
    MEngine       *loadedEngine         = nullptr;

    MEngine       &MEngine::Get()
    {
        return *loadedEngine;
    }

    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void MEngine::Init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;
        m_Log        = std::make_unique<Log>();

        JobSystem::Init(3);

        m_Window = std::make_shared<WindowCore>();

        m_ContextDevice = std::make_unique<VkContextDevice>(m_Window.get());

        m_MainCamera = std::make_shared<Camera>();
        m_Scene      = std::make_shared<Scene>();

        JobSystem::Context contextJobs;
        JobSystem::Execute(contextJobs, [this](auto args) { 
            MaterialAllocator::Init(); 
        });

        JobSystem::Execute(contextJobs, [this](auto args)
            {
                m_Renderer = std::make_unique<Renderer>(*m_ContextDevice.get(), m_Window);
                InitImgui();
                m_Renderer->InitSceneRenderer(m_MainCamera, m_Scene);

            });
        
        JobSystem::Wait(contextJobs);

        m_IsInitialized = true;

        m_MainDeletionQueue.PushFunction([&]() { 
            m_ContextDevice->DestroyFrameData(); 
        });

    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};
        bool      isResized{false};
        bool      isStopRendering{false};

        JobSystem::Context eventContext;

        PROFILE_SETTHREADNAME("Main Thread");

        while (!bQuit)
        {
            while (SDL_PollEvent(&event) != 0)
            {
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                ImGui_ImplSDL3_ProcessEvent(&event);

                const auto typeEvent = event.window.type;

                if (typeEvent == SDL_EVENT_WINDOW_RESIZED)
                {
                    isResized = true;
                }

                if (typeEvent == SDL_EVENT_WINDOW_MINIMIZED)
                {
                    isStopRendering = true;
                }
                if (typeEvent == SDL_EVENT_WINDOW_RESTORED)
                {
                    isStopRendering = false;
                }


                m_MainCamera->ProccessEvent(event);

                m_InputEvent = &event;
            }

            if (isStopRendering)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_ContextDevice->IsResizeRequest() || isResized)
            {
                m_ContextDevice->ResizeSwapchain(m_Window->Resize());
            }

            if (!m_ContextDevice->IsResizeRequest())
            {
                m_GuiLayer->ImGuiRender();

                const float deltaTime = ImGui::GetIO().DeltaTime;

                UpdateScene(deltaTime);

                m_Renderer->Render();
            }
        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            m_Log.reset();
            VkDevice &device = LogicalDevice::GetDevice();
            vkDeviceWaitIdle(device);
            m_MainDeletionQueue.Flush();

            m_Renderer.reset();

            m_GuiLayer->Deactivate();
            m_GuiLayer.reset();

            m_Scene.reset();

            MaterialAllocator::Destroy();

            m_Window.reset();
            m_ContextDevice.reset();

            JobSystem::Release();
        }

        loadedEngine = nullptr;
    }

    void MEngine::UpdateScene(float inDeltaTime)
    {
        m_MainCamera->Update(inDeltaTime);
        m_Scene->Update();
        m_Renderer->UpdateSceneRenderer(inDeltaTime);
    }

    void MEngine::InitImgui()
    {
        m_GuiLayer->Init();

        m_Renderer->InitImGuiRenderer();

        m_MainDeletionQueue.PushFunction([=]() { ImGui_ImplVulkan_Shutdown(); });
    }

    void MEngine::PushGuiLayer(ImGuiLayer *inLayer)
    {
        m_GuiLayer = std::unique_ptr<ImGuiLayer>(inLayer);
    }
} // namespace MamontEngine

