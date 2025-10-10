#include "Renderer.h"
#include "Core/ContextDevice.h"

#include "Core/VkInitializers.h"
#include "Core/VkImages.h"
#include "Core/VkPipelines.h"
#include "Core/Window.h"
#include "Core/ImGuiRenderer.h"

#include "Utils/Profile.h"
#include "Graphics/Vulkan/Allocator.h"

#include "Graphics/Devices/LogicalDevice.h"
//#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace
{
    std::array<glm::vec3, 8> CalcFrustumCornersWorldSpace(const glm::mat4 &view_proj)
    {
        std::array<glm::vec3, 8>           corners{};
        const glm::mat4          inv_view_proj            = glm::inverse(view_proj);
        constexpr glm::vec3                untransformed_corners[8] = {
                glm::vec3(-1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, 1.0f, 0.0f),
                glm::vec3(1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, -1.0f, 0.0f),
                glm::vec3(-1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, 1.0f, 1.0f),
                glm::vec3(1.0f, -1.0f, 1.0f),
                glm::vec3(-1.0f, -1.0f, 1.0f),
        };
        for (size_t i = 0; i < 8; i++)
        {
            const glm::vec4 pt = inv_view_proj * glm::vec4(untransformed_corners[i], 1.0);
            corners[i]         = glm::vec3(pt / pt.w);
        }
        return corners;
    }

    glm::mat4 GetLightMatrix(float                  near_plane,
                             float                  far_plane,
                             float                  fov_degrees,
                             float                  aspect_ratio,
                             const glm::mat4       &cam_view,
                             const glm::vec3       &light_dir,
                             [[maybe_unused]] float z_mult)
    {
        glm::mat4 proj_matrix = glm::perspective(glm::radians(fov_degrees), aspect_ratio, near_plane, far_plane);
        proj_matrix[1][1] *= -1;
        auto frustum_corners = CalcFrustumCornersWorldSpace(proj_matrix * cam_view);
        // avg the frustum corner positions to get the center of the frustum
        glm::vec3 center{0};
        for (const glm::vec3 &corner : frustum_corners)
        {
            center += corner;
        }
        center /= 8;
        // light is looking at center of frustum from its direction
        // glm::vec3 up = glm::abs(light_dir.y) > 0.99f ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
        glm::vec3 up         = {0.f, 1.f, 0.f};
        glm::mat4 light_view = glm::lookAt(center, center + light_dir, up);

        // fit the frustum with orthographic projection matrix
        float min_x = std::numeric_limits<float>::max();
        float max_x = std::numeric_limits<float>::lowest();
        float min_y = std::numeric_limits<float>::max();
        float max_y = std::numeric_limits<float>::lowest();
        float min_z = std::numeric_limits<float>::max();
        float max_z = std::numeric_limits<float>::lowest();
        for (const auto &corner : frustum_corners)
        {
            const glm::vec4 transformed_corner = light_view * glm::vec4(corner, 1.0);
            min_x                              = std::min(min_x, transformed_corner.x);
            max_x                              = std::max(max_x, transformed_corner.x);
            min_y                              = std::min(min_y, transformed_corner.y);
            max_y                              = std::max(max_y, transformed_corner.y);
            min_z                              = std::min(min_z, transformed_corner.z);
            max_z                              = std::max(max_z, transformed_corner.z);
        }
        if (z_mult > 0.0)
        {
            if (min_z < 0)
            {
                min_z *= z_mult;
            }
            else
            {
                min_z /= z_mult;
            }
            if (max_z < 0)
            {
                max_z /= z_mult;
            }
            else
            {
                max_z *= z_mult;
            }
        }

        return glm::orthoRH_ZO(min_x, max_x, max_y, min_y, min_z, max_z) * light_view;

        // return glm::ortho(min_x, max_x, max_y, min_y, max_z, min_z) * light_view;
    }
}

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Renderer::Renderer(VkContextDevice &inDeviceContext, const std::shared_ptr<WindowCore> &inWindow) : m_DeviceContext(inDeviceContext), m_Window(inWindow)
    {
    }

    void Renderer::Clear()
    {
        m_SceneRenderer->Clear();
    }

    void Renderer::InitSceneRenderer(const std::shared_ptr<Camera> &inMainCamera)
    {
        m_SceneRenderer = std::make_shared<SceneRenderer>(inMainCamera);
        m_SceneRenderer->UpdateCascades(m_DeviceContext.Cascades);
    }

    void Renderer::InitImGuiRenderer(VkDescriptorPool &outDescPool)
    {
        m_ImGuiRenderer = std::make_unique<ImGuiRenderer>(m_DeviceContext, m_Window->GetWindow(), m_DeviceContext.Swapchain.GetImageFormat(), outDescPool);
    }

    void Renderer::InitPipelines()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        const std::pair<VkFormat, VkFormat> inImageFormats{m_DeviceContext.Image.DrawImage.ImageFormat, m_DeviceContext.Image.DepthImage.ImageFormat};
        m_RenderPipeline = std::make_unique<RenderPipeline>(device, m_DeviceContext.GPUSceneDataDescriptorLayout, inImageFormats);
        m_SkyPipeline    = std::make_unique<SkyPipeline>(device, &m_DeviceContext.DrawImageDescriptorLayout);

        m_DeviceContext.RenderPipeline = m_RenderPipeline.get();

        //CreateShadowPipeline();
    }

    void Renderer::Render()
    {
        PROFILE_ZONE("Renderer::Render");

        auto    &currentFrame = m_DeviceContext.GetCurrentFrame();
        VkDevice device       = LogicalDevice::GetDevice();

        if (vkGetFenceStatus(device, currentFrame.RenderFence) != VK_SUCCESS)
        {
            VK_CHECK_MESSAGE(vkWaitForFences(device, 1, &currentFrame.RenderFence, VK_TRUE, UINT64_MAX), "Wait FENCE");
        }

        const auto [resultAcquire, swapchainImageIndex] = m_DeviceContext.Swapchain.AcquireImage(device, currentFrame.SwapchainSemaphore);
        if (resultAcquire == VK_ERROR_OUT_OF_DATE_KHR || resultAcquire == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            return;
        }

        const VkExtent2D swapchainExtent = m_DeviceContext.Swapchain.GetExtent();
        m_DrawExtent.height              = std::min(swapchainExtent.height, m_DeviceContext.Image.DrawImage.ImageExtent.height) * m_RenderScale;
        m_DrawExtent.width               = std::min(swapchainExtent.width, m_DeviceContext.Image.DrawImage.ImageExtent.width) * m_RenderScale;

        VkCommandBuffer cmd = currentFrame.MainCommandBuffer;

        VK_CHECK(vkResetFences(device, 1, &currentFrame.RenderFence));
        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        VkUtil::transition_image(
                cmd, m_DeviceContext.Image.DepthImage.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

        DrawMain(cmd);

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

        VkImage currentSwapchainImage = m_DeviceContext.Swapchain.GetImageAt(swapchainImageIndex);
        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        VkUtil::copy_image_to_image(cmd, m_DeviceContext.Image.DrawImage.Image, currentSwapchainImage, m_DrawExtent, m_DeviceContext.Swapchain.GetExtent());

        VkUtil::transition_image(cmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_UNDEFINED);

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        DrawImGui(cmd, m_DeviceContext.Swapchain.GetImageView(swapchainImageIndex));

        VkUtil::transition_image(cmd, currentSwapchainImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
        //std::cerr << "Renderer, m_DeviceContext.Image.DrawImage.Image: " << m_DeviceContext.Image.DrawImage.Image << std::endl;

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkCommandBufferSubmitInfo cmdInfo  = vkinit::command_buffer_submit_info(cmd);
        const VkSemaphoreSubmitInfo     waitInfo = vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR,
                                                                             m_DeviceContext.GetFrameAt(swapchainImageIndex).SwapchainSemaphore);
        const VkSemaphoreSubmitInfo     signalInfo =
                vkinit::semaphore_submit_info(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, m_DeviceContext.RenderCopleteSemaphores[swapchainImageIndex]);

        const VkSubmitInfo2 submit = vkinit::submit_info(&cmdInfo, &signalInfo, &waitInfo);
        VK_CHECK(vkQueueSubmit2(m_DeviceContext.GetGraphicsQueue(), 1, &submit, currentFrame.RenderFence));

        const VkResult presentResult = m_DeviceContext.Swapchain.Present(
                m_DeviceContext.GetGraphicsQueue(), &m_DeviceContext.RenderCopleteSemaphores[swapchainImageIndex], swapchainImageIndex);
        if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            m_IsResizeRequested = true;
            return;
        }

        m_DeviceContext.IncrementFrameNumber();
    }

    void Renderer::DrawMain(VkCommandBuffer inCmd)
    {
        PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "DrawMain");

        {
            PROFILE_VK_ZONE(m_DeviceContext.GetCurrentFrame().TracyContext, inCmd, "Draw SkyPipeline");
            m_SkyPipeline->Draw(inCmd, &m_DeviceContext.DrawImageDescriptors);
        }

        const auto start = std::chrono::high_resolution_clock::now();

        DrawGeometry(inCmd);

        const auto end     = std::chrono::high_resolution_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

        m_Stats.MeshDrawTime = elapsed.count() / 1000.f;
    }

    void Renderer::DrawGeometry(VkCommandBuffer inCmd)
    {
        auto &currentFrame = m_DeviceContext.GetCurrentFrame();
        PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Draw Geometry");

        UpdateUniformBuffers();

        const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();

        if (IsActiveCascade)
        {
            VkUtil::transition_image(
                    inCmd, m_DeviceContext.CascadeDepthImage.Image.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

            const VkExtent2D cascadeExtent = {.width = SHADOWMAP_DIMENSION, .height = SHADOWMAP_DIMENSION};

            for (size_t i = 0; i < CASCADECOUNT; i++)
            {
                VkImageView cascadeDepth = m_DeviceContext.Cascades[i].View;
                if (cascadeDepth == VK_NULL_HANDLE)
                {
                    fmt::println("cascadeDepth is Null");
                    continue;
                }
                VkRenderingAttachmentInfo depthAttachment = vkinit::depth_attachment_info(cascadeDepth, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
                depthAttachment.clearValue.depthStencil.depth = 1.0;
                depthAttachment.loadOp                        = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthAttachment.storeOp                       = VK_ATTACHMENT_STORE_OP_STORE;

                const VkRenderingInfo renderCascadeInfo = vkinit::rendering_info(cascadeExtent, nullptr, &depthAttachment);

                vkCmdBeginRendering(inCmd, &renderCascadeInfo);
                vkCmdSetDepthBias(inCmd, 1.25f, 0.f, 1.75f);
                SetViewportScissor(inCmd, cascadeExtent);
                vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Pipeline);

                /*vkCmdBindDescriptorSets(inCmd,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS, m_CascadePipeline->Layout,
                                        0,
                                       1,
                                        &currentFrame.GlobalDescriptor,
                                        0,
                                        nullptr);*/
                m_SceneRenderer->RenderShadow(
                        inCmd, currentFrame.GlobalDescriptor, m_DeviceContext.Cascades[i].ViewProjectMatrix, m_CascadePipeline->Layout, static_cast<uint32_t>(i));
                
                vkCmdEndRendering(inCmd);
            }

            VkUtil::transition_image(
                    inCmd, m_DeviceContext.CascadeDepthImage.Image.Image, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        }

        const VkExtent2D extent = m_Window->GetExtent();


        /*VkUtil::transition_image(
                inCmd, m_DeviceContext.Image.DrawImage.Image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/

        const VkRenderingAttachmentInfo colorAttachment =
                vkinit::attachment_info(m_DeviceContext.Image.DrawImage.ImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingAttachmentInfo depthAttachment =
                vkinit::depth_attachment_info(m_DeviceContext.Image.DepthImage.ImageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
        //std::cerr << "m_DeviceContext.Image.DepthImage.ImageView: " << m_DeviceContext.Image.DepthImage.ImageView << std::endl;

        const VkRenderingInfo renderInfo = vkinit::rendering_info(extent, &colorAttachment, &depthAttachment);
        vkCmdDispatch(inCmd, std::ceil(extent.width / 16.0), std::ceil(extent.height / 16.0), 1);

        vkCmdBeginRendering(inCmd, &renderInfo);

        SetViewportScissor(inCmd, m_DrawExtent);
        vkCmdSetDepthBias(inCmd, 0, 0, 0);
        {
            PROFILE_VK_ZONE(currentFrame.TracyContext, inCmd, "Scene Render");
            /*vkCmdBindDescriptorSets(inCmd,
                                    VK_PIPELINE_BIND_POINT_GRAPHICS, m_DeviceContext.RenderPipeline->OpaquePipeline->Layout,
                                    0,
                                    1,
                                    &currentFrame.GlobalDescriptor,
                                    0,
                                    nullptr);*/
            m_SceneRenderer->Render(inCmd, currentFrame.GlobalDescriptor, sceneData.Viewproj, m_DeviceContext.RenderPipeline->OpaquePipeline->Layout);
        }
        vkCmdEndRendering(inCmd);

        /*stats.DrawCallCount = 0;
        stats.TriangleCount = 0;*/
    }

    void Renderer::SetViewportScissor(VkCommandBuffer cmd, const VkExtent2D &inExtent) const
    {
        const VkViewport viewport = {0, 0, (float)inExtent.width, (float)inExtent.height, 0.f, 1.f};
        const VkRect2D   scissor  = {{0, 0}, {inExtent.width, inExtent.height}};

        vkCmdSetViewport(cmd, 0, 1, &viewport);
        vkCmdSetScissor(cmd, 0, 1, &scissor);
    }

    void Renderer::DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView)
    {
        m_ImGuiRenderer->Draw(inCmd, inTargetImageView, m_DeviceContext.Swapchain.GetExtent());
    }

    void Renderer::UpdateUniformBuffers()
    {
        const auto&  currentFrame    = m_DeviceContext.GetCurrentFrame();
        void        *sceneBufferData = currentFrame.SceneDataBuffer.GetMappedData();

        const GPUSceneData &sceneData = m_SceneRenderer->GetGPUSceneData();
        std::memcpy(sceneBufferData, &sceneData, sizeof(GPUSceneData));

        /*std::array<glm::mat4, CASCADECOUNT> cascadeViewProjMatrices{};
        for (size_t i = 0; i < CASCADECOUNT; i++)
        {
            cascadeViewProjMatrices[i] = m_DeviceContext.Cascades[i].ViewProjectMatrix;
        }
        void *cascadeViewData = currentFrame.CascadeViewProjMatrices.GetMappedData();
        std::memcpy(cascadeViewData, cascadeViewProjMatrices.data(), sizeof(glm::mat4) * CASCADECOUNT);

        

        void       *cascadeBufferData = currentFrame.FragmentBuffer.GetMappedData();
        const auto &sceneFragmentData = m_SceneRenderer->GetSceneFragment();
        std::memcpy(cascadeBufferData, &sceneFragmentData, sizeof(SceneDataFragment));*/
    }

    void Renderer::ResizeSwapchain()
    {
        m_IsResizeRequested = true;

        fmt::println("Resize Swapchain");

        m_DeviceContext.ResizeSwapchain(m_Window->Resize());

        m_IsResizeRequested = false;
    }

    void Renderer::UpdateWindowEvent(const SDL_EventType inType)
    {
        if (inType == SDL_EVENT_WINDOW_RESIZED)
        {
            m_IsResizeRequested = true;
        }

        if (inType == SDL_EVENT_WINDOW_MINIMIZED)
        {
            m_StopRendering = true;
        }
        if (inType == SDL_EVENT_WINDOW_RESTORED)
        {
            m_StopRendering = false;
        }
    }

    void Renderer::UpdateSceneRenderer()
    {
        m_SceneRenderer->UpdateCascades(m_DeviceContext.Cascades);

        m_SceneRenderer->Update(m_Window->GetExtent(), m_DeviceContext.Cascades);

        //m_SceneRenderer->UpdateCascades(m_DeviceContext.Cascades);
    }

    void Renderer::DestroyPipelines()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        m_RenderPipeline->Destroy(device);
        m_SkyPipeline->Destroy(device);
        //vkDestroyPipelineLayout(device, m_CascadePipeline->Layout, nullptr);
        //vkDestroyPipeline(device, m_CascadePipeline->Pipeline, nullptr);
    }

/// Shadows


    void Renderer::CreateShadowPipeline()
    {
        constexpr VkPushConstantRange matrixRange{
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            .offset = 0, 
            .size = sizeof(GPUDrawPushConstants)
        };
        const VkDevice device = LogicalDevice::GetDevice();

        const std::array<VkDescriptorSetLayout, 2> layouts = {m_DeviceContext.GPUSceneDataDescriptorLayout, m_DeviceContext.RenderPipeline->Layout};

        VkPipelineLayoutCreateInfo layoutInfo = vkinit::pipeline_layout_create_info();
        layoutInfo.setLayoutCount                   = static_cast<uint32_t>(layouts.size());
        layoutInfo.pSetLayouts                      = layouts.data();
        layoutInfo.pPushConstantRanges              = &matrixRange;
        layoutInfo.pushConstantRangeCount           = 1;

        VkPipelineLayout layout;
        VK_CHECK(vkCreatePipelineLayout(device, &layoutInfo, nullptr, &layout));

        const std::string cascadeShadowPath = RootDirectories + "/MamontEngine/src/Shaders/cascade_shadow.vert.spv";

        VkShaderModule cascadeShadowShader;
        if (!VkPipelines::LoadShaderModule(cascadeShadowPath.c_str(), device, &cascadeShadowShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }
        const std::string cascadeFragmentShadowPath = RootDirectories + "/MamontEngine/src/Shaders/cascade_shadow.frag.spv";

        VkShaderModule cascadeFragmentShadowShader;
        if (!VkPipelines::LoadShaderModule(cascadeFragmentShadowPath.c_str(), device, &cascadeFragmentShadowShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.Clear();
//        pipelineBuilder.SetVertexInput(vertexInputInfo);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetLayout(layout);
        pipelineBuilder.SetShaders(cascadeShadowShader, cascadeFragmentShadowShader);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.SetDepthFormat(m_DeviceContext.CascadeDepthImage.Image.ImageFormat);
        //pipelineBuilder.SetColorAttachmentFormat(VK_FORMAT_UNDEFINED);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.DisableBlending();
  //      pipelineBuilder.SetDepthBiasEnable(VK_TRUE);
        pipelineBuilder.EnableDepthClamp(VK_TRUE);
      //  pipelineBuilder.AddDynamicState(VK_DYNAMIC_STATE_DEPTH_BIAS);

        VkPipeline shadowPipeline = pipelineBuilder.BuildPipline(device/*, 0*/);
        if (shadowPipeline == VK_NULL_HANDLE)
        {
            fmt::println("shadowPipeline == VK_NULL_HANDLE");
        }
        m_CascadePipeline       = std::make_unique<PipelineData>(shadowPipeline, layout);

        vkDestroyShaderModule(device, cascadeShadowShader, nullptr);
        vkDestroyShaderModule(device, cascadeFragmentShadowShader, nullptr);

        pipelineBuilder.Clear();
    }

    void Renderer::CalculateShadowData(const glm::mat4& inCameraView, const glm::vec4 inLightDirection)
    {
        m_ShadowCascadeUBOData.PlaneDistances[0] = m_Settings.Z_Far / 50.0f;
        m_ShadowCascadeUBOData.PlaneDistances[1]    = m_Settings.Z_Far / 25.0f;
        m_ShadowCascadeUBOData.PlaneDistances[2] = m_Settings.Z_Far / 10.0f;
        m_ShadowCascadeUBOData.PlaneDistances[3]    = m_Settings.Z_Far / 2.0f;
        std::array<float, CASCADECOUNT - 1> levels{};
        for (size_t i = 0; i < 4; i++)
        {
            levels[i] = m_ShadowCascadeUBOData.PlaneDistances[i];
        }

        CalculateLightSpaceMatrices(levels, inLightDirection, m_Settings.Shadow.ZMult);
    }

    void Renderer::CalculateLightSpaceMatrices(std::span<float> inCascadeLevels, const glm::vec4 inLightDirection, float zMult)
    {
        const glm::vec3 direction = glm::normalize(inLightDirection);
        const auto      camera                 = m_SceneRenderer->GetCamera();
        const auto      cameraView             = camera->GetViewMatrix();
        const auto      aspectRatio            = camera->GetAspectRatio();
        const auto      fov                    = camera->GetFOV();
        m_ShadowMatrices.LightSpaceMatrices[0] = GetLightMatrix(camera->GetNearClip(), inCascadeLevels[0], 
            camera->GetFOV(), camera->GetAspectRatio(), camera->GetViewMatrix(), inLightDirection, zMult);

        const size_t levelsSize = inCascadeLevels.size();
        for (size_t i = 1; i < levelsSize; i++)
        {
            m_ShadowMatrices.LightSpaceMatrices[i] =
                    GetLightMatrix(inCascadeLevels[i - 1], inCascadeLevels[i], fov, aspectRatio, cameraView, inLightDirection, zMult);
        }

        m_ShadowMatrices.LightSpaceMatrices[levelsSize] =
                GetLightMatrix(inCascadeLevels.back(), camera->GetFarClip(), fov, aspectRatio, cameraView, inLightDirection, zMult);
    }


} // namespace MamontEngine
