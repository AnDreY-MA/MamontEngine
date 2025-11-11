#include "Engine.h"

#include "Graphics/Vulkan/Swapchain.h"
#include "ImGuiRenderer.h"

#include "ECS/Entity.h"
#include <ECS/Components/MeshComponent.h>

#include "Utils/Profile.h"
#include "tracy/public/TracyClient.cpp"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;
    MEngine       *loadedEngine         = nullptr;
    MEngine       &MEngine::Get()
    {
        return *loadedEngine;
#include "ECS/Scene.h"
    }

    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void MEngine::Init()
    {
        assert(loadedEngine == nullptr);
        loadedEngine = this;

        m_Log = std::make_unique<Log>();

        m_Window = std::make_shared<WindowCore>();

        m_ContextDevice = std::make_unique<VkContextDevice>(m_Window.get());

        m_Renderer = std::make_unique<Renderer>(*m_ContextDevice.get(), m_Window);

        m_MainDeletionQueue.PushFunction([&]() { 
            m_ContextDevice->DestroyFrameData(); 
        });

        InitPipelines();    

        InitImgui();

        InitDefaultData();

        InitScene();

        m_MainCamera->SetVelocity(glm::vec3(0.f));
        //m_MainCamera->SetPosition(glm::vec3(1, 1, 0));
        m_IsInitialized = true;
    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};

        tracy::SetThreadName("Main Thread");
        //FrameMarkStart("MainLoop");

        while (!bQuit)
        {
            //const auto start = std::chrono::high_resolution_clock::now();

            while (SDL_PollEvent(&event) != 0)
            { 
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                ImGui_ImplSDL3_ProcessEvent(&event);
                
                m_Renderer->UpdateWindowEvent(event.window.type);

                m_MainCamera->ProccessEvent(event);

                m_InputEvent = &event;
            }

            if (m_Renderer->IsStopRendering())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_Renderer->IsResizeRequested())
            {
                m_Renderer->ResizeSwapchain();
                //bResize = false;
               /* std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;*/
            }

            if (!m_Renderer->IsResizeRequested())
            {
                {
                    PROFILE_ZONE("Editor render");

                    m_GuiLayer->Begin();

                    m_GuiLayer->ImGuiRender();

                    m_GuiLayer->End();
                }

                const float deltaTime = ImGui::GetIO().DeltaTime;

                UpdateScene(deltaTime);
                m_Renderer->Render();
                //Draw();
            }

            /*const auto end     = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            stats.FrameTime = elapsed.count() / 1000.f;*/

        }
        //FrameMarkEnd("MainLoop");

    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            m_Log.release();
            const VkDevice device = LogicalDevice::GetDevice();
            vkDeviceWaitIdle(device);
            
            m_GuiLayer->Deactivate();
            
            m_Renderer->Clear();
            
            m_MainDeletionQueue.Flush();

            m_ContextDevice->Swapchain.Destroy(device);

            m_ContextDevice.release();
            //~ContextDevice

            //m_Window->Close();
        }

        loadedEngine = nullptr;
    }

    void MEngine::InitDefaultData()
    {
        m_ContextDevice->InitSamples();

        m_ContextDevice->InitDefaultImages();


        m_MainDeletionQueue.PushFunction(
                [&]()
                {
                    const VkDevice device = LogicalDevice::GetDevice();
                    vkDestroySampler(device, m_ContextDevice->DefaultSamplerNearest, nullptr);
                    vkDestroySampler(device, m_ContextDevice->DefaultSamplerLinear, nullptr);
                });
    }
    
    void MEngine::InitScene()
    {
        m_MainCamera    = std::make_shared<Camera>();
        m_Renderer->InitSceneRenderer(m_MainCamera);
        m_Scene = std::make_shared<Scene>(m_Renderer->GetSceneRenderer());
        m_Scene->Init(*m_ContextDevice);
    }

    void MEngine::UpdateScene(float inDeltaTime)
    {
        m_MainCamera->Update(inDeltaTime);
        m_Scene->Update();
        m_Renderer->UpdateSceneRenderer(inDeltaTime);

    }
    
    VkFormat MEngine::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (const VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(PhysicalDevice::GetDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat MEngine::FindDepthFormat() const
    {
        return FindSupportedFormat({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
                                   VK_IMAGE_TILING_OPTIMAL,
                                   VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    }

    void MEngine::InitPipelines()
    {
        m_Renderer->InitPipelines();

        m_MainDeletionQueue.PushFunction([=]() { 
            m_Renderer->DestroyPipelines();
            });
    }
    
    void MEngine::InitImgui()
    {   
        m_GuiLayer->Init();

        m_Renderer->InitImGuiRenderer();

        m_MainDeletionQueue.PushFunction(
                [=]()
                {
                    ImGui_ImplVulkan_Shutdown();
                });

    }

    void MEngine::PushGuiLayer(ImGuiLayer *inLayer)
    {
        m_GuiLayer = std::unique_ptr<ImGuiLayer>(inLayer);
        
    }
    
}