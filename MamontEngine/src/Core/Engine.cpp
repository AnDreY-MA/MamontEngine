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
#include "imgui/imgui_impl_sdl3.h"
#include "imgui/imgui_impl_vulkan.h"
#include <glm/gtx/transform.hpp>


#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

static SDL_Window* window = nullptr;
VkPipeline         gradientPipeline;

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

        SDL_Init(SDL_INIT_VIDEO);

        SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

        window = SDL_CreateWindow(
            "Mamont Engine", 
            m_WindowExtent.width,
            m_WindowExtent.height,
            window_flags);

        m_Window = window;

        InitVulkan();
        
        InitSwapchain();
        
        InitCommands();
        
        InitSyncStructeres();
        
        InitDescriptors();

        InitPipelines();    

        InitDefaultData();

        InitRenderable();

        InitImgui();

        m_MainCamera.SetVelocity(glm::vec3(0.f));
        m_MainCamera.SetPosition(glm::vec3(0, 0, 0));

        m_IsInitialized = true;
    }

    void MEngine::Run()
    {
        SDL_Event event;
        bool      bQuit{false};

        while (!bQuit)
        {
            const auto start = std::chrono::system_clock::now();

            while (SDL_PollEvent(&event) != 0)
            { 
                if (event.type == SDL_EVENT_QUIT)
                    bQuit = true;

                m_MainCamera.ProccessEvent(event);
                ImGui_ImplSDL3_ProcessEvent(&event);

                if (event.window.type == SDL_EVENT_WINDOW_MINIMIZED)
                {
                    m_StopRendering = true;
                }
                if (event.window.type == SDL_EVENT_WINDOW_RESTORED)
                {
                    m_StopRendering = false;
                }

            }

            if (m_StopRendering)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }

            if (m_IsResizeRequested)
            {
                ResizeSwapchain();
            }

            ImGui_ImplVulkan_NewFrame();
            ImGui_ImplSDL3_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Stats");

                ImGui::Text("frametime %f ms", stats.FrameTime);
                ImGui::Text("drawtime %f ms", stats.MeshDrawTime);
                ImGui::Text("triangles %i", stats.TriangleCount);
                ImGui::Text("draws %i", stats.DrawCallCount);
            ImGui::End();

            if (ImGui::Begin("Background"))
            {
                ImGui::SliderFloat("Render Scale", &m_RenderScale, 0.3f, 1.f);

                ComputeEffect &selected = m_BackgroundEffects[m_CurrentBackgroundEffect];
                ImGui::Text("Selected Effect: ", selected.Name);

                ImGui::SliderInt("Effect Index", &m_CurrentBackgroundEffect, 0, m_BackgroundEffects.size() - 1);

                ImGui::InputFloat4("data1", (float *)&selected.Data.Data1);
                ImGui::InputFloat4("data2", (float *)&selected.Data.Data2);
                ImGui::InputFloat4("data3", (float *)&selected.Data.Data3);
                ImGui::InputFloat4("data4", (float *)&selected.Data.Data4);
                ImGui::End();

            }

            ImGui::Render();

            UpdateScene();

            Draw();

            auto end     = std::chrono::system_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

            stats.FrameTime = elapsed.count() / 1000.f;
        }
    }

    void MEngine::Cleanup()
    {
        if (m_IsInitialized)
        {
            vkDeviceWaitIdle(m_Device);
            loadedScenes.clear();

            for (auto& frame : m_Frames)
            {
                frame.Deleteions.Flush();
            }
            
            m_MainDeletionQueue.Flush();

            DestroySwapchain();

            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);

            //vmaDestroyAllocator(m_Allocator);

            vkDestroyDevice(m_Device, nullptr);
            vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
            vkDestroyInstance(m_Instance, nullptr);

            SDL_DestroyWindow(window);
            m_Window = nullptr;
        }

        loadedEngine = nullptr;
    }

    void MEngine::InitDefaultData()
    {
        std::array<Vertex, 4> rect_vertices{};

        rect_vertices[0].Position = { 0.5, -0.5, 0};
        rect_vertices[1].Position = { 0.5, 0.5, 0};
        rect_vertices[2].Position = { -0.5, -0.5, 0};
        rect_vertices[3].Position = { -0.5, 0.5, 0};

        rect_vertices[0].Color = { 0, 0, 0, 1};
        rect_vertices[1].Color = { 0.5, 0.5, 0.5, 1};
        rect_vertices[2].Color = { 1, 0, 0, 1};
        rect_vertices[3].Color = { 0, 1, 0, 1};

        rect_vertices[0].UV_X = 1;
        rect_vertices[0].UV_Y = 0;
        rect_vertices[1].UV_X = 0;
        rect_vertices[1].UV_Y = 0;
        rect_vertices[2].UV_X = 1;
        rect_vertices[2].UV_Y = 1;
        rect_vertices[3].UV_X = 0;
        rect_vertices[3].UV_Y = 1;

        std::array<uint32_t, 6> rect_indices{};

        rect_indices[0] = 0;
        rect_indices[1] = 1;
        rect_indices[2] = 2;

        rect_indices[3] = 2;
        rect_indices[4] = 1;
        rect_indices[5] = 3;

        uint32_t white = glm::packUnorm4x8(glm::vec4(1, 1, 1, 1));
        m_WhiteImage    = CreateImage((void *)&white, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t grey = glm::packUnorm4x8(glm::vec4(0.66f, 0.66f, 0.66f, 1));
        m_GreyImage   = CreateImage((void *)&grey, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        uint32_t black = glm::packUnorm4x8(glm::vec4(0, 0, 0, 0));
        m_BlackImage   = CreateImage((void *)&black, VkExtent3D{1, 1, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        // checkerboard image
        uint32_t                      magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
        std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
        for (int x = 0; x < 16; x++)
        {
            for (int y = 0; y < 16; y++)
            {
                pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
            }
        }
        m_ErrorCheckerboardImage = CreateImage(pixels.data(), VkExtent3D{16, 16, 1}, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT);

        VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

        sampl.magFilter = VK_FILTER_NEAREST;
        sampl.minFilter = VK_FILTER_NEAREST;

        vkCreateSampler(m_Device, &sampl, nullptr, &m_DefaultSamplerNearest);

        sampl.magFilter = VK_FILTER_LINEAR;
        sampl.minFilter = VK_FILTER_LINEAR;

        vkCreateSampler(m_Device, &sampl, nullptr, &m_DefaultSamplerLinear);

        m_MainDeletionQueue.PushFunction(
                [&]()
                {
                    vkDestroySampler(m_Device, m_DefaultSamplerNearest, nullptr);
                    vkDestroySampler(m_Device, m_DefaultSamplerLinear, nullptr);

                    DestroyImage(m_WhiteImage);
                    DestroyImage(m_GreyImage);
                    DestroyImage(m_BlackImage);
                    DestroyImage(m_ErrorCheckerboardImage);
                });

        GLTFMetallic_Roughness::MaterialResources materialResources;
        // default the material textures
        materialResources.ColorImage        = m_WhiteImage;
        materialResources.ColorSampler      = m_DefaultSamplerLinear;
        materialResources.MetalRoughImage   = m_WhiteImage;
        materialResources.MetalRoughSampler = m_DefaultSamplerLinear;

        // set the uniform buffer for the material data
        AllocatedBuffer materialConstants =
                CreateBuffer(sizeof(GLTFMetallic_Roughness::MaterialConstants), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // write the buffer
        GLTFMetallic_Roughness::MaterialConstants *sceneUniformData =
                (GLTFMetallic_Roughness::MaterialConstants *)materialConstants.Allocation->GetMappedData();
        sceneUniformData->ColorFactors        = glm::vec4{1, 1, 1, 1};
        sceneUniformData->Metal_rough_factors = glm::vec4{1, 0.5, 0, 0};

        m_MainDeletionQueue.PushFunction([=, this]() { DestroyBuffer(materialConstants); });

        materialResources.DataBuffer       = materialConstants.Buffer;
        materialResources.DataBufferOffset = 0;

        m_DefualtDataMatInstance = m_MetalRoughMaterial.WriteMaterial(m_Device, EMaterialPass::MAIN_COLOR, materialResources, m_GlobalDescriptorAllocator);

    }
    
    void MEngine::Draw()
    {
        VK_CHECK_MESSAGE(vkWaitForFences(m_Device, 1, &m_Frames[m_FrameNumber].RenderFence, VK_TRUE, 1000000000), "Wait FENCE");

        m_Frames[m_FrameNumber].Deleteions.Flush();
        m_Frames[m_FrameNumber].FrameDescriptors.ClearPools(m_Device);

        uint32_t swapchainImageIndex;
        VkResult e = vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, m_Frames[m_FrameNumber].SwapchainSemaphore, nullptr, &swapchainImageIndex);
        if (e == VK_ERROR_OUT_OF_DATE_KHR)
        {
            m_IsResizeRequested = true;
            return;
        } 

        m_DrawExtent.height = std::min(m_SwapchainExtent.height, m_DrawImage.ImageExtent.height) * m_RenderScale;
        m_DrawExtent.width  = std::min(m_SwapchainExtent.width, m_DrawImage.ImageExtent.width) * m_RenderScale;

        VK_CHECK(vkResetFences(m_Device, 1, &m_Frames[m_FrameNumber].RenderFence));
        VK_CHECK(vkResetCommandBuffer(m_Frames[m_FrameNumber].MainCommandBuffer, 0));
          
        VkCommandBuffer cmd = m_Frames[m_FrameNumber].MainCommandBuffer;
        
        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);

        /*DrawBackground(cmd);

        VkUtil::transition_image(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/
        VkUtil::transition_image(cmd, m_DepthImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        VkUtil::transition_image(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        //< draw_first
    
        //> imgui_draw

        VkUtil::copy_image_to_image(cmd, m_DrawImage.Image, m_SwapchainImages[swapchainImageIndex], m_DrawExtent, m_SwapchainExtent);

        VkUtil::transition_image(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        DrawImGui(cmd, m_SwapchainImageViews[swapchainImageIndex]);

        VkUtil::transition_image(cmd, m_SwapchainImages[swapchainImageIndex], VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

        VK_CHECK(vkEndCommandBuffer(cmd));
        //< imgui_draw

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSemaphoreSubmitInfo     waitInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, m_Frames[m_FrameNumber].SwapchainSemaphore);
        VkSemaphoreSubmitInfo signalInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_Frames[m_FrameNumber].RenderSemaphore);

        VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_Frames[m_FrameNumber].RenderFence));

        // Prepare present
        VkPresentInfoKHR presentInfo = vkinit::present_info();
        presentInfo.pSwapchains      = &m_Swapchain;
        presentInfo.swapchainCount   = 1;

        presentInfo.pWaitSemaphores    = &m_Frames[m_FrameNumber].RenderSemaphore;
        presentInfo.waitSemaphoreCount = 1;

        presentInfo.pImageIndices = &swapchainImageIndex;

        VkResult presentResult = vkQueuePresentKHR(m_GraphicsQueue, &presentInfo);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            //fmt::println("Resize Request");
            m_IsResizeRequested = true;
            return;
        }

        m_FrameNumber = (m_FrameNumber + 1) % FRAME_OVERLAP;
    }

    void MEngine::DrawBackground(VkCommandBuffer inCmd)
    {
        ComputeEffect &effect = m_BackgroundEffects[m_CurrentBackgroundEffect];

        // bind the background compute pipeline
        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.Pipeline);

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipelineLayout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);

        vkCmdPushConstants(inCmd, m_GradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.Data);

        vkCmdDispatch(inCmd, std::ceil(m_DrawExtent.width / 16.0), std::ceil(m_DrawExtent.height / 16.0), 1);
    }

    void MEngine::DrawMain(VkCommandBuffer inCmd)
    {
        ComputeEffect &effect = m_BackgroundEffects[m_CurrentBackgroundEffect];

        vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, effect.Pipeline);

        vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_GradientPipelineLayout, 0, 1, &m_DrawImageDescriptors, 0, nullptr);

        vkCmdPushConstants(inCmd, m_GradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstants), &effect.Data);
        vkCmdDispatch(inCmd, std::ceil(m_WindowExtent.width / 16.0), std::ceil(m_WindowExtent.height / 16.0), 1);

        VkUtil::transition_image(inCmd, m_DrawImage.Image, VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(m_DrawImage.ImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(m_DepthImage.ImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        VkRenderingInfo renderInfo = vkinit::rendering_info(m_WindowExtent, &colorAttachment, &depthAttachment);

        vkCmdBeginRendering(inCmd, &renderInfo);

        auto start = std::chrono::system_clock::now();

        DrawGeometry(inCmd);

        auto end     = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        stats.MeshDrawTime = elapsed.count() / 1000.f;

        vkCmdEndRendering(inCmd);
    }

    static bool is_visible(const RenderObject &obj, const glm::mat4 &viewproj)
    {
        std::array<glm::vec3, 8> corners{
                glm::vec3{1, 1, 1},
                glm::vec3{1, 1, -1},
                glm::vec3{1, -1, 1},
                glm::vec3{1, -1, -1},
                glm::vec3{-1, 1, 1},
                glm::vec3{-1, 1, -1},
                glm::vec3{-1, -1, 1},
                glm::vec3{-1, -1, -1},
        };

        glm::mat4 matrix = viewproj * obj.Transform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 v = matrix * glm::vec4(obj.Bound.Origin + (corners[c] * obj.Bound.Extents), 1.f);

            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
        {
            return false;
        }
        else
        {
            return true;
        }
    }

    void MEngine::DrawGeometry(VkCommandBuffer inCmd)
    {
        std::vector<uint32_t> opaque_draws;
        opaque_draws.reserve(m_MainDrawContext.OpaqueSurfaces.size());

        for (int i = 0; i < m_MainDrawContext.OpaqueSurfaces.size(); i++)
        {
            if (is_visible(m_MainDrawContext.OpaqueSurfaces[i], m_SceneData.Viewproj))
            {
                opaque_draws.push_back(i);
            }
        }

        std::vector<uint32_t> transp_draws;
        transp_draws.reserve(m_MainDrawContext.TransparentSurfaces.size());

        for (int i = 0; i < m_MainDrawContext.TransparentSurfaces.size(); i++)
        {
            if (is_visible(m_MainDrawContext.TransparentSurfaces[i], m_SceneData.Viewproj))
            {
                transp_draws.push_back(i);
            }
        }

        // allocate a new uniform buffer for the scene data
        AllocatedBuffer gpuSceneDataBuffer = CreateBuffer(sizeof(GPUSceneData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        // add it to the deletion queue of this frame so it gets deleted once its been used
        m_Frames[m_FrameNumber].Deleteions.PushFunction([=, this]() { DestroyBuffer(gpuSceneDataBuffer); });

        // write the buffer
        GPUSceneData *sceneUniformData = (GPUSceneData *)gpuSceneDataBuffer.Allocation->GetMappedData();
        *sceneUniformData              = m_SceneData;

        VkDescriptorSet globalDescriptor = m_Frames[m_FrameNumber].FrameDescriptors.Allocate(m_Device, m_GPUSceneDataDescriptorLayout/*, &allocArrayInfo*/);

        DescriptorWriter writer;
        writer.WriteBuffer(0, gpuSceneDataBuffer.Buffer, sizeof(GPUSceneData), 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        writer.UpdateSet(m_Device, globalDescriptor);

        MaterialPipeline *lastPipeline    = nullptr;
        MaterialInstance *lastMaterial    = nullptr;
        VkBuffer          lastIndexBuffer = VK_NULL_HANDLE;

        auto draw = [&](const RenderObject &r)
        {
            if (r.Material != lastMaterial)
            {
                lastMaterial = r.Material;
                if (r.Material->Pipeline != lastPipeline)
                {

                    lastPipeline = r.Material->Pipeline;
                    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Pipeline);
                    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

                    VkViewport viewport = {};
                    viewport.x          = 0;
                    viewport.y          = 0;
                    viewport.width      = (float)m_DrawExtent.width;
                    viewport.height     = (float)m_DrawExtent.height;
                    viewport.minDepth   = 0.f;
                    viewport.maxDepth   = 1.f;

                    vkCmdSetViewport(inCmd, 0, 1, &viewport);

                    VkRect2D scissor      = {};
                    scissor.offset.x      = 0;
                    scissor.offset.y      = 0;
                    scissor.extent.width  = m_DrawExtent.width;
                    scissor.extent.height = m_DrawExtent.height;

                    vkCmdSetScissor(inCmd, 0, 1, &scissor);
                }

                vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);
            }
            if (r.IndexBuffer != lastIndexBuffer)
            {
                lastIndexBuffer = r.IndexBuffer;
                vkCmdBindIndexBuffer(inCmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }
            // calculate final mesh matrix
            GPUDrawPushConstants push_constants;
            push_constants.WorldMatrix  = r.Transform;
            push_constants.VertexBuffer = r.VertexBufferAddress;

            vkCmdPushConstants(inCmd, r.Material->Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

            stats.DrawCallCount++;
            stats.TriangleCount += r.IndexCount / 3;
            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };

        stats.DrawCallCount = 0;
        stats.TriangleCount = 0;

        for (auto &r : opaque_draws)
        {
            draw(m_MainDrawContext.OpaqueSurfaces[r]);
        }

        for (auto &r : transp_draws)
        {
            draw(m_MainDrawContext.TransparentSurfaces[r]);
        }

        m_MainDrawContext.OpaqueSurfaces.clear();
        m_MainDrawContext.TransparentSurfaces.clear();

    }

    void MEngine::DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView) const
    {
        VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkRenderingInfo           renderInfo      = vkinit::rendering_info(m_SwapchainExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(inCmd, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmd);

        vkCmdEndRendering(inCmd);
    }
    
    void MEngine::InitRenderable()
    {
        const std::string structurePath = {RootDirectories + "/MamontEngine/assets/house.glb"};
        auto        structureFile = loadGltf(this, structurePath);

        assert(structureFile.has_value());

        loadedScenes["structure"] = *structureFile;
    }

    void MEngine::UpdateScene()
    {
        m_MainCamera.Update();

        glm::mat4 view = m_MainCamera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(70.f), (float)m_WindowExtent.width / (float)m_WindowExtent.height, 10000.f, 0.1f);
        projection[1][1] *= -1;

        m_SceneData.View = view;
        m_SceneData.Proj = projection;
        m_SceneData.Viewproj = projection * view;

        for (auto &scene : loadedScenes)
        {
            scene.second->Draw(glm::mat4{1.f}, m_MainDrawContext);
        }

    }
    
    void MEngine::InitVulkan()
    {
        vkb::InstanceBuilder builder;

        auto instRet = builder.set_app_name("Mamont App")
            .request_validation_layers(bUseValidationLayers)
            .use_default_debug_messenger()
            .require_api_version(1, 3, 290)
            .build();

        vkb::Instance vkbInstance = instRet.value();
        m_Instance                = vkbInstance.instance;
        m_DebugMessenger          = vkbInstance.debug_messenger;

        if (!m_Instance || !m_DebugMessenger)
        {
            fmt::println("m_Instance || m_DebugMessenger is NULL");
        }

        if (!SDL_Vulkan_CreateSurface((SDL_Window*)m_Window, m_Instance, nullptr, &m_Surface))
        {
            fmt::print("Surface is false");
            return;
        }

        VkPhysicalDeviceVulkan13Features features{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
        features.dynamicRendering = true;
        features.synchronization2 = true;
    
        VkPhysicalDeviceVulkan12Features features12{.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
        features12.bufferDeviceAddress               = true;
        features12.descriptorIndexing                = true;
        features12.shaderInt8                        = true;
        features12.uniformAndStorageBuffer8BitAccess = true;
        features12.runtimeDescriptorArray            = true;


        vkb::PhysicalDeviceSelector selector{vkbInstance};

        vkb::PhysicalDevice         physicalDevice = selector
            .set_minimum_version(1, 3)
            .set_required_features_13(features)
            .set_required_features_12(features12)
                                                     .set_surface(m_Surface)
                                                     //.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
            .select()
            .value();

        vkb::DeviceBuilder deviceBuilder{physicalDevice};

        vkb::Device vkbDevice = deviceBuilder.build().value();

        m_Device = vkbDevice.device;
        m_ChosenGPU = physicalDevice.physical_device;
        fmt::print("Name {}", physicalDevice.name);

        m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
        m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice         = m_ChosenGPU;
        allocatorInfo.device                 = m_Device;
        allocatorInfo.instance               = m_Instance;
        allocatorInfo.flags                  = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vmaCreateAllocator(&allocatorInfo, &m_Allocator);

        m_MainDeletionQueue.PushFunction([&]() { 
            vmaDestroyAllocator(m_Allocator); 
            });

    }
    
    void MEngine::InitSwapchain()
    {
        CreateSwapchain(m_WindowExtent.width, m_WindowExtent.height);

        VkExtent3D drawImageExtent = {m_WindowExtent.width, m_WindowExtent.height, 1};

        m_DrawImage.ImageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        m_DrawImage.ImageExtent = drawImageExtent;

        VkImageUsageFlags drawImageUsages{};
        drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
        drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        VkImageCreateInfo rImageInfo = vkinit::image_create_info(m_DrawImage.ImageFormat, drawImageUsages, drawImageExtent);

        VmaAllocationCreateInfo rImageAllocInfo = {};
        rImageAllocInfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
        rImageAllocInfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        vmaCreateImage(m_Allocator, &rImageInfo, &rImageAllocInfo, &m_DrawImage.Image, &m_DrawImage.Allocation, nullptr);

        VkImageViewCreateInfo rViewInfo = vkinit::imageview_create_info(m_DrawImage.ImageFormat, m_DrawImage.Image, VK_IMAGE_ASPECT_COLOR_BIT);

        VK_CHECK(vkCreateImageView(m_Device, &rViewInfo, nullptr, &m_DrawImage.ImageView));

    //> depthimg
        m_DepthImage.ImageFormat = VK_FORMAT_D32_SFLOAT;
        m_DepthImage.ImageExtent = drawImageExtent;
        VkImageUsageFlags depthImageUsages{};
        depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

        VkImageCreateInfo dimgInfo = vkinit::image_create_info(m_DepthImage.ImageFormat, depthImageUsages, drawImageExtent);
        vmaCreateImage(m_Allocator, &dimgInfo, &rImageAllocInfo, &m_DepthImage.Image, &m_DepthImage.Allocation, nullptr);

        VkImageViewCreateInfo dViewInfo = vkinit::imageview_create_info(m_DepthImage.ImageFormat, m_DepthImage.Image, VK_IMAGE_ASPECT_DEPTH_BIT);

        VK_CHECK(vkCreateImageView(m_Device, &dViewInfo, nullptr, &m_DepthImage.ImageView));
    //< depthimg

        m_MainDeletionQueue.PushFunction([=]() { 
                    vkDestroyImageView(m_Device, m_DrawImage.ImageView, nullptr);
                    vmaDestroyImage(m_Allocator, m_DrawImage.Image, m_DrawImage.Allocation);

                    vkDestroyImageView(m_Device, m_DepthImage.ImageView, nullptr);
                    vmaDestroyImage(m_Allocator, m_DepthImage.Image, m_DepthImage.Allocation);
            });

    }
    
    void MEngine::ResizeSwapchain()
    {
        vkDeviceWaitIdle(m_Device);

        DestroySwapchain();

        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        m_WindowExtent.width  = w;
        m_WindowExtent.height = h;

        CreateSwapchain(m_WindowExtent.width, m_WindowExtent.height);

        m_IsResizeRequested = false;
    }

    void MEngine::DestroySwapchain()
    {
        vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

        for (int i = 0; i < m_SwapchainImageViews.size(); i++)
        {
            vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
        }
    } 

    void MEngine::InitCommands()
    {
        VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_Frames[i].CommandPool));

            VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_Frames[i].CommandPool, 1);

            VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].MainCommandBuffer));
        
            m_MainDeletionQueue.PushFunction([=]() { vkDestroyCommandPool(m_Device, m_Frames[i].CommandPool, nullptr);
                });
        }

        VK_CHECK(vkCreateCommandPool(m_Device, &commandPoolInfo, nullptr, &m_ImmCommandPool));

        VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(m_ImmCommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_ImmCommandBuffer));

        m_MainDeletionQueue.PushFunction([=]() { 
                vkDestroyCommandPool(m_Device, m_ImmCommandPool, nullptr);
        });

    }
    
    void MEngine::InitSyncStructeres()
    {
        VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

        VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_ImmFence));
        m_MainDeletionQueue.PushFunction([=]() { 
            vkDestroyFence(m_Device, m_ImmFence, nullptr); 
            });

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            VK_CHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].RenderFence));

            VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();

            VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].SwapchainSemaphore));
            VK_CHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].RenderSemaphore));
            
            m_MainDeletionQueue.PushFunction([=]() { 
                    vkDestroyFence(m_Device, m_Frames[i].RenderFence, nullptr);
                    vkDestroySemaphore(m_Device, m_Frames[i].SwapchainSemaphore, nullptr);
                    vkDestroySemaphore(m_Device, m_Frames[i].RenderSemaphore, nullptr);
            });
        }

    }

    void MEngine::CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight)
    {
        vkb::SwapchainBuilder swapchainBuilder{m_ChosenGPU, m_Device, m_Surface};
        m_SwapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

        vkb::Swapchain vkbSwapchain = swapchainBuilder
            .set_desired_format(VkSurfaceFormatKHR{.format = m_SwapchainImageFormat, .colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR})
                        .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_extent(inWidth, inHeight)
            .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .build()
            .value();

        m_SwapchainExtent = vkbSwapchain.extent;
        m_Swapchain       = vkbSwapchain.swapchain;
        m_SwapchainImages = vkbSwapchain.get_images().value();
        m_SwapchainImageViews = vkbSwapchain.get_image_views().value();

        fmt::println("CreateSwapchain");
    }
    
    void MEngine::InitPipelines()
    {
        InitBackgrounPipeline();

        m_MetalRoughMaterial.BuildPipelines(this);

    }
    
    void MEngine::InitBackgrounPipeline()
    {
        VkPipelineLayoutCreateInfo computeLayout{};
        computeLayout.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        computeLayout.pNext          = nullptr;
        computeLayout.pSetLayouts    = &m_DrawImageDescriptorLayout;
        computeLayout.setLayoutCount = 1;

        VkPushConstantRange pushConstant{};
        pushConstant.offset     = 0;
        pushConstant.size       = sizeof(ComputePushConstants);
        pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        computeLayout.pPushConstantRanges    = &pushConstant;
        computeLayout.pushConstantRangeCount = 1;

        VK_CHECK(vkCreatePipelineLayout(m_Device, &computeLayout, nullptr, &m_GradientPipelineLayout));

        std::string    shaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/gradient_color.comp.spv";
        VkShaderModule gradientShader;
        if (!VkPipelines::LoadShaderModule(shaderPath.c_str(), m_Device, &gradientShader))
        {
            fmt::print("Error when building the Gradient shader \n");
        }

        VkShaderModule skyShader;
        std::string    skyShaderPath = std::string(PROJECT_ROOT_DIR) + "/MamontEngine/src/Shaders/sky.comp.spv";
        if (!VkPipelines::LoadShaderModule(skyShaderPath.c_str(), m_Device, &skyShader))
        {
            fmt::print("Error when building the Sky shader ");
        }


        VkPipelineShaderStageCreateInfo stageinfo{};
        stageinfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageinfo.pNext  = nullptr;
        stageinfo.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        stageinfo.module = gradientShader;
        stageinfo.pName  = "main";

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.pNext  = nullptr;
        computePipelineCreateInfo.layout = m_GradientPipelineLayout;
        computePipelineCreateInfo.stage  = stageinfo;

        ComputeEffect gradient;
        gradient.Layout = m_GradientPipelineLayout;
        gradient.Name   = "gradient";
        gradient.Data   = {};

        gradient.Data.Data1 = glm::vec4(1, 0, 0, 1);
        gradient.Data.Data2 = glm::vec4(0, 0, 1, 1);

        VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &gradient.Pipeline));

        computePipelineCreateInfo.stage.module = skyShader;

        ComputeEffect sky;
        sky.Layout     = m_GradientPipelineLayout;
        sky.Name       = "Sky";
        sky.Data       = {};
        sky.Data.Data1 = glm::vec4(0.1, 0.2, 0.4, 0.97);

        VK_CHECK(vkCreateComputePipelines(m_Device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &sky.Pipeline));

        m_BackgroundEffects.push_back(gradient);
        m_BackgroundEffects.push_back(sky);

        vkDestroyShaderModule(m_Device, gradientShader, nullptr);
        vkDestroyShaderModule(m_Device, skyShader, nullptr);
        m_MainDeletionQueue.PushFunction(
                [=]()
                {
                    vkDestroyPipelineLayout(m_Device, m_GradientPipelineLayout, nullptr);
                    vkDestroyPipeline(m_Device, sky.Pipeline, nullptr);
                    vkDestroyPipeline(  m_Device, gradient.Pipeline, nullptr);
                });
    }
    
    void MEngine::InitImgui()
    {
        VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                             {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                             {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                             {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags                       = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets                     = 1000;
        poolInfo.poolSizeCount               = (uint32_t)std::size(poolSizes);
        poolInfo.pPoolSizes                  = poolSizes;

        VkDescriptorPool imguiPool;
        VK_CHECK_MESSAGE(vkCreateDescriptorPool(m_Device, &poolInfo, nullptr, &imguiPool), "CreateDescPool");

        ImGui::CreateContext();

        ImGui_ImplSDL3_InitForVulkan(window);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = m_Instance;
        initInfo.PhysicalDevice            = m_ChosenGPU;
        initInfo.Device                    = m_Device;
        initInfo.Queue                     = m_GraphicsQueue;
        initInfo.DescriptorPool            = imguiPool;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering       = true;

        initInfo.PipelineRenderingCreateInfo = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &m_SwapchainImageFormat;

        ImGui_ImplVulkan_Init(&initInfo);

        ImGui_ImplVulkan_CreateFontsTexture();

        m_MainDeletionQueue.PushFunction(
                [=]()
                {
                    ImGui_ImplVulkan_Shutdown();
                    vkDestroyDescriptorPool(m_Device, imguiPool, nullptr);
                });

       
    }
    
    void MEngine::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& inFunction) const
    {
        VK_CHECK(vkResetFences(m_Device, 1, &m_ImmFence));
        VK_CHECK(vkResetCommandBuffer(m_ImmCommandBuffer, 0));

        VkCommandBuffer cmd = m_ImmCommandBuffer;

        VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(m_GraphicsQueue, 1, &submit, m_ImmFence));

        VK_CHECK(vkWaitForFences(m_Device, 1, &m_ImmFence, true, 9999999999));
    }
    
    void MEngine::InitDescriptors()
    {
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
                {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
        };

        m_GlobalDescriptorAllocator.Init(m_Device, 10, sizes);

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
            m_DrawImageDescriptorLayout = builder.Build(m_Device, VK_SHADER_STAGE_COMPUTE_BIT);
        }

        {
            DescriptorLayoutBuilder builder;
            builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            m_GPUSceneDataDescriptorLayout = builder.Build(m_Device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
        }

        m_DrawImageDescriptors = m_GlobalDescriptorAllocator.Allocate(m_Device, m_DrawImageDescriptorLayout);

        DescriptorWriter writer;
        writer.WriteImage(0, m_DrawImage.ImageView, VK_NULL_HANDLE, VK_IMAGE_LAYOUT_GENERAL, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(m_Device, m_DrawImageDescriptors);

        m_MainDeletionQueue.PushFunction(
                [&]()
                {
                    m_GlobalDescriptorAllocator.DestroyPools(m_Device);

                    vkDestroyDescriptorSetLayout(m_Device, m_GPUSceneDataDescriptorLayout, nullptr);
                    vkDestroyDescriptorSetLayout(m_Device, m_DrawImageDescriptorLayout, nullptr);
                });

        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
                    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
                    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
                    {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
            };

            m_Frames[i].FrameDescriptors = DescriptorAllocatorGrowable{};
            m_Frames[i].FrameDescriptors.Init(m_Device, 1000, frame_sizes);

            m_MainDeletionQueue.PushFunction([&, i]() { 
                m_Frames[i].FrameDescriptors.DestroyPools(m_Device); 
            });
        }
    }
    
    AllocatedBuffer MEngine::CreateBuffer(size_t inAllocSize, VkBufferUsageFlags inUsage, VmaMemoryUsage inMemoryUsage) const
    {
        VkBufferCreateInfo bufferInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        bufferInfo.pNext              = nullptr;
        bufferInfo.size               = inAllocSize;
        bufferInfo.usage              = inUsage;

        VmaAllocationCreateInfo vmaAllocInfo = {};
        vmaAllocInfo.usage                   = inMemoryUsage;
        vmaAllocInfo.flags                   = VMA_ALLOCATION_CREATE_MAPPED_BIT;
        AllocatedBuffer newBuffer{};

        VK_CHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &vmaAllocInfo, &newBuffer.Buffer, &newBuffer.Allocation, &newBuffer.Info));

        return newBuffer;
    }

    void MEngine::DestroyBuffer(const AllocatedBuffer &inBuffer) const
    {
        vmaDestroyBuffer(m_Allocator, inBuffer.Buffer, inBuffer.Allocation);
    }

    GPUMeshBuffers MEngine::UploadMesh(std::span<uint32_t> inIndices, std::span<Vertex> inVertices)
    {
        const size_t vertexBufferSize{inVertices.size() * sizeof(Vertex)};
        const size_t indexBufferSize{inIndices.size() * sizeof(uint32_t)};  

        GPUMeshBuffers newSurface;

        newSurface.VertexBuffer = CreateBuffer(vertexBufferSize,
                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                             VMA_MEMORY_USAGE_GPU_ONLY);

        VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = newSurface.VertexBuffer.Buffer};
        newSurface.VertexBufferAddress              = vkGetBufferDeviceAddress(m_Device, &deviceAddressInfo);

        newSurface.IndexBuffer = CreateBuffer(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging = CreateBuffer(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        void *data = staging.Allocation->GetMappedData();

        memcpy(data, inVertices.data(), vertexBufferSize);
        memcpy((char *)data + vertexBufferSize, inIndices.data(), indexBufferSize);

        ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkBufferCopy vertexCopy{0};
                    vertexCopy.dstOffset = 0;
                    vertexCopy.srcOffset = 0;
                    vertexCopy.size      = vertexBufferSize;

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.VertexBuffer.Buffer, 1, &vertexCopy);

                    VkBufferCopy indexCopy{0};
                    indexCopy.dstOffset = 0;
                    indexCopy.srcOffset = vertexBufferSize;
                    indexCopy.size      = indexBufferSize;

                    vkCmdCopyBuffer(cmd, staging.Buffer, newSurface.IndexBuffer.Buffer, 1, &indexCopy);
                });

        DestroyBuffer(staging);

        return newSurface;
    
    }

    AllocatedImage MEngine::CreateImage(VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped) const
    {
        AllocatedImage newImage;
        newImage.ImageFormat = inFormat;
        newImage.ImageExtent = inSize;

        VkImageCreateInfo img_info = vkinit::image_create_info(inFormat, inUsage, inSize);
        if (inIsMipMapped)
        {
            img_info.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(inSize.width, inSize.height)))) + 1;
        }

        // always allocate images on dedicated GPU memory
        VmaAllocationCreateInfo allocinfo = {};
        allocinfo.usage                   = VMA_MEMORY_USAGE_GPU_ONLY;
        allocinfo.requiredFlags           = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        VK_CHECK(vmaCreateImage(m_Allocator, &img_info, &allocinfo, &newImage.Image, &newImage.Allocation, nullptr));


        VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
        if (inFormat == VK_FORMAT_D32_SFLOAT)
        {
            aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo view_info       = vkinit::imageview_create_info(inFormat, newImage.Image, aspectFlag);
        view_info.subresourceRange.levelCount = img_info.mipLevels;

        VK_CHECK(vkCreateImageView(m_Device, &view_info, nullptr, &newImage.ImageView));

        return newImage;
    }
    
    AllocatedImage MEngine::CreateImage(void *inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped)
    {
        size_t          data_size    = inSize.depth * inSize.width * inSize.height * 4;
        AllocatedBuffer uploadbuffer = CreateBuffer(data_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

        memcpy(uploadbuffer.Info.pMappedData, inData, data_size);

        AllocatedImage new_image = CreateImage(inSize, inFormat, inUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, inIsMipMapped);

        ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    VkUtil::transition_image(cmd, new_image.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                    VkBufferImageCopy copyRegion = {};
                    copyRegion.bufferOffset      = 0;
                    copyRegion.bufferRowLength   = 0;
                    copyRegion.bufferImageHeight = 0;

                    copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
                    copyRegion.imageSubresource.mipLevel       = 0;
                    copyRegion.imageSubresource.baseArrayLayer = 0;
                    copyRegion.imageSubresource.layerCount     = 1;
                    copyRegion.imageExtent                     = inSize;

                    vkCmdCopyBufferToImage(cmd, uploadbuffer.Buffer, new_image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

                    if (inIsMipMapped)
                    {
                        VkUtil::generate_mipmaps(cmd, new_image.Image, VkExtent2D{new_image.ImageExtent.width, new_image.ImageExtent.height});
                    }
                    else
                    {
                        VkUtil::transition_image(cmd, new_image.Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                    }

                });

        DestroyBuffer(uploadbuffer);

        return new_image;
    }
    
    void MEngine::DestroyImage(const AllocatedImage &inImage)
    {
        vkDestroyImageView(m_Device, inImage.ImageView, nullptr);
        vmaDestroyImage(m_Allocator, inImage.Image, inImage.Allocation);
    }
}