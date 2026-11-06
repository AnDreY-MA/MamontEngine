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
#include "Graphics/Resources/Textures/TextureManager.h"
#include "Core/JobSystem.h"
#include "Core/Event/EventSystem.h"
#include "Core/TimeStep.h"
#include "Core/Log.h"

#include "Audio/AudioSystem.h"

#include "Asset/AssetManager.h"


namespace MamontEngine
{
    MEngine       *loadedEngine         = nullptr;

    MEngine       &MEngine::Get()
    {
        return *loadedEngine;
    }

    MEngine::MEngine() = default;
    MEngine::~MEngine() = default;

    const std::string RootDirectories = PROJECT_ROOT_DIR;

    static void InputMessagesCallback(void* event, void* userData)
    {
        auto *input = static_cast<InputSystem *>(userData);
        input->OnEvent(event);
    }

    void MEngine::Init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        m_Log        = std::make_unique<Log>();

        m_TimeStep = new TimeStep();

        JobSystem::Init(3);

        m_Window = std::make_shared<WindowCore>();
        m_InputSystem = std::make_unique<InputSystem>();
        m_Window->RegisterMessagesCallbacks(InputMessagesCallback, m_InputSystem.get());

        m_ContextDevice = std::make_unique<VkContextDevice>(m_Window.get());

        TextureManager::GetInstance().Init();

        m_Scene      = std::make_shared<Scene>();

        JobSystem::Context contextJobs;
        JobSystem::Execute(contextJobs, [this](auto args) { 
            m_MaterialManager = std::make_unique<MaterialManager>();
        });

        JobSystem::Execute(contextJobs, [this](auto args)
            {
                m_Renderer = std::make_unique<Renderer>(*m_ContextDevice.get(), m_Window);
                m_Renderer->InitSceneRenderer(m_Scene);
            });

        JobSystem::Execute(contextJobs,
                           [this](auto args) { 
                                m_PhysicsSystem = std::make_unique<HeroPhysics::PhysicsSystem>();
                           });
        
        JobSystem::Wait(contextJobs);
        InitImgui();

        AssetMananger::Get().OnModelLoaded.Add([](Asset *inAsset) { 
                Log::Info("OnModelLoading: {}", inAsset->GetPathFile());
            });

        m_ScriptSystem = std::make_unique<ScriptSystem>();

        m_AudioSystem = std::make_unique<AudioSystem>();

        m_IsInitialized = true;

        m_MainDeletionQueue.PushFunction([&]() { 
            m_ContextDevice->DestroyFrameData(); 
        });
    }

    void MEngine::Run()
    {
        PROFILE_SETTHREADNAME("Main Thread");

        MaterialManager::Get()->SetupDefaults();
        while (!m_Window->IsRequestQuit())
        {
            const auto deltaTime = ImGui::GetIO().DeltaTime;

            {
                m_TimeStep->Update();

                ImGuiIO &io = ImGui::GetIO();
                io.DeltaTime = (float)m_TimeStep->GetSeconds();

                const auto frameTime = m_TimeStep->GetMillis();
            }

            InputSystem::Get().NewFrame();

            m_Window->PollEvents();
            
            if (m_Window->IsMinimized())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_ContextDevice->IsResizeRequest())
            {
                Log::Info("Resized");
                m_ContextDevice->ResizeSwapchain(m_Window->Resize());
            }
            else
            {
                m_PhysicsSystem->Update(deltaTime);

                UpdateScene(deltaTime);

                m_Renderer->Render();
            }
        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            VkDevice &device = LogicalDevice::GetDevice();
            vkDeviceWaitIdle(device);
            
            m_Window->UnregisterMessagesCallbacks(InputMessagesCallback);

            m_GuiLayer->Deactivate();
            m_GuiLayer.reset();

            ImGui_ImplVulkan_Shutdown();

            m_Scene.reset();
            
            m_Renderer.reset();

            m_MaterialManager.reset();
            TextureManager::GetInstance().Shutdown();

            m_MainDeletionQueue.Flush();

            m_Window.reset();

            m_ContextDevice.reset();

            JobSystem::Release();

            m_Log.reset();

            delete m_TimeStep;
        }

        loadedEngine = nullptr;
    }

    void MEngine::UpdateScene(float inDeltaTime)
    {
        PROFILE_FUNCTION();


        m_Renderer->UpdateSceneRenderer(inDeltaTime);

        m_Scene->Update(inDeltaTime);
    }

    std::chrono::milliseconds MEngine::CalculateDeltaTimeMs()
    {
        const uint64_t currentTime =
                static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());

        if (m_LastFrameTime == 0)
        {
            m_LastFrameTime = currentTime;
            return std::chrono::milliseconds(16);
        }

        const uint64_t delta = currentTime - m_LastFrameTime;

        m_LastFrameTime = currentTime;

        return std::chrono::milliseconds(static_cast<long long>(delta));
    }

    void MEngine::InitImgui()
    {
        m_GuiLayer->Init();

        m_Renderer->InitImGuiRenderer(m_GuiLayer);

        //m_MainDeletionQueue.PushFunction([]() { ImGui_ImplVulkan_Shutdown(); });
    }

    void MEngine::PushGuiLayer(ImGuiLayer *inLayer)
    {
        m_GuiLayer = std::shared_ptr<ImGuiLayer>(inLayer);
    }

    void MEngine::ReloadShaders()
    {
        m_Renderer->InitPipelines();
    }

    AudioSystem &MEngine::GetAudioSystem() const
    {
        return *m_AudioSystem.get();
    }
} // namespace MamontEngine

