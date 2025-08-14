#include "Engine.h"

#include "VkInitializers.h"
#include "VkImages.h"
#include "VkPipelines.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_events.h>

#include <chrono>
#include <thread>
#include <VkBootstrap.h>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include <glm/gtx/transform.hpp>
#include "Graphics/Vulkan/Swapchain.h"
#include "ImGuiRenderer.h"

#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include <ECS/Components/MeshComponent.h>

#include "ImGuizmo/ImGuizmo.h"

namespace MamontEngine
{
    constexpr bool bUseValidationLayers = false;
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

        m_Window = std::make_shared<WindowCore>();

        m_ContextDevice = std::make_unique<VkContextDevice>();

        m_Renderer = std::make_unique<Renderer>(*m_ContextDevice.get(), m_Window);

        InitSwapchain();

        InitCommands();

        InitSyncStructeres();
        
        InitDescriptors();

        InitPipelines();    

        InitImgui();

        InitDefaultData();

        InitScene();

        //m_Window->SetupVulkanWindow(*m_ContextDevice, m_ContextDevice->Surface, 1, 1);

        m_ContextDevice->CreateAllFrameOffscreans(m_ContextDevice->Swapchain.GetExtent(), m_ContextDevice->Swapchain.GetImageFormat(), m_ContextDevice->Swapchain.GetImageViews());

        m_MainCamera->SetVelocity(glm::vec3(0.f));
        m_MainCamera->SetPosition(glm::vec3(1, 1, 0));
        m_IsInitialized = true;
    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};

        while (!bQuit)
        {
            //const auto start = std::chrono::high_resolution_clock::now();

            while (SDL_PollEvent(&event) != 0)
            { 
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                m_Renderer->UpdateWindowEvent(event.window.type);

                m_MainCamera->ProccessEvent(event);

                ImGui_ImplSDL3_ProcessEvent(&event);
            }

            if (m_Renderer->IsStopRendering())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                continue;
            }

            if (m_Renderer->IsResizeRequested())
            {
                ResizeSwapchain();
            }

            if (!m_Renderer->IsResizeRequested())
            {
                m_GuiLayer->Begin();

                m_GuiLayer->ImGuiRender();

                m_GuiLayer->End();

                UpdateScene();
                m_Renderer->Render();
                //Draw();
            }

            /*const auto end     = std::chrono::high_resolution_clock::now();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            stats.FrameTime = elapsed.count() / 1000.f;*/
        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            vkDeviceWaitIdle(m_ContextDevice->Device);
            
            m_GuiLayer->Deactivate();
            
            m_Renderer->Clear();
            
            m_MainDeletionQueue.Flush();

            m_ContextDevice->Swapchain.Destroy(m_ContextDevice->Device);

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
                    vkDestroySampler(m_ContextDevice->Device, m_ContextDevice->DefaultSamplerNearest, nullptr);
                    vkDestroySampler(m_ContextDevice->Device, m_ContextDevice->DefaultSamplerLinear, nullptr);

                    m_ContextDevice->DestroyImage(m_ContextDevice->WhiteImage);
                    m_ContextDevice->DestroyImage(m_ContextDevice->ErrorCheckerboardImage);
                });
    }
    
    void MEngine::InitScene()
    {
        m_MainCamera    = std::make_shared<Camera>();
        m_Renderer->InitSceneRenderer(m_MainCamera);
        m_Scene = std::make_shared<Scene>(m_Renderer->GetSceneRenderer());
        m_Scene->Init(*m_ContextDevice);
    }

    void MEngine::UpdateScene()
    {
        m_MainCamera->Update();
        m_Scene->Update();
        m_Renderer->UpdateSceneRenderer();

    }
    
    void MEngine::InitVulkan()
    {
        m_ContextDevice->Init(m_Window.get());
    }
    
    void MEngine::InitSwapchain()
    {
        m_ContextDevice->InitSwapchain(m_Window->GetExtent());

        m_MainDeletionQueue.PushFunction([=]() { 
                m_ContextDevice->DestroyImage();   
        });

    }

    void MEngine::ResizeSwapchain()
    {
        fmt::println("Resize Swapchain");

        m_ContextDevice->ResizeSwapchain(m_Window->Resize());

        m_IsResizeRequested = false;
    }

    VkFormat MEngine::FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const
    {
        for (const VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_ContextDevice->ChosenGPU, format, &props);

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

    void MEngine::InitCommands()
    {
        m_ContextDevice->InitCommands(m_MainDeletionQueue);

        // Viewport Command Pool
        /*m_ImGuiRenderer.CreateViewportCommandPool(m_ContextDevice->Device, m_ContextDevice->GraphicsQueueFamily);
        m_MainDeletionQueue.PushFunction([=]() { 
            vkDestroyCommandPool(m_ContextDevice->Device, m_ImGuiRenderer.GetViewportCommandPool(), nullptr); 
            });*/

    }
    
    void MEngine::InitSyncStructeres()
    {
        m_ContextDevice->InitSyncStructeres();

        m_MainDeletionQueue.PushFunction([=]() {
                m_ContextDevice->DestroySyncStructeres();
            });
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
        VkDescriptorPool outImguiPool;
        m_Renderer->InitImGuiRenderer(outImguiPool);

        m_MainDeletionQueue.PushFunction(
                [=]()
                {
                    ImGui_ImplVulkan_Shutdown();
                    vkDestroyDescriptorPool(m_ContextDevice->Device, outImguiPool, nullptr);
                });

    }

    void MEngine::PushGuiLayer(ImGuiLayer *inLayer)
    {
        m_GuiLayer = std::unique_ptr<ImGuiLayer>(inLayer);
        m_GuiLayer->Init();
        
    }
    
    void MEngine::InitDescriptors()
    {
        m_ContextDevice->InitDescriptors();

        m_MainDeletionQueue.PushFunction([&]() { 
                    m_ContextDevice->DestroyDescriptors();
                });
    }

    
}