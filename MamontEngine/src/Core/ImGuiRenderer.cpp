#include "ImGuiRenderer.h"
#include "ContextDevice.h"
#include "VkDestriptor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "FrameData.h"

#include "VkInitializers.h"

#include "Engine.h"


namespace MamontEngine
{
    ImGuiRenderer::ImGuiRenderer(VkContextDevice &inContextDevice, SDL_Window *inWindow, VkFormat inColorFormat, VkDescriptorPool &outPoolResult)
    {
        constexpr VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
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

        const VkDescriptorPoolCreateInfo poolInfo = {.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                     .pNext         = nullptr,
                                                     .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                                     .maxSets       = 1000,
                                                     .poolSizeCount = (uint32_t)std::size(poolSizes),
                                                     .pPoolSizes    = poolSizes};
        VkDescriptorPool                 imguiPool;
        VK_CHECK_MESSAGE(vkCreateDescriptorPool(inContextDevice.Device, &poolInfo, nullptr, &imguiPool), "CreateDescPool");
        outPoolResult = imguiPool;

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = inContextDevice.Instance;
        initInfo.PhysicalDevice            = inContextDevice.GetPhysicalDevice();
        initInfo.Device                    = inContextDevice.Device;
        initInfo.QueueFamily               = inContextDevice.GraphicsQueueFamily;
        initInfo.Queue                     = inContextDevice.GraphicsQueue;
        initInfo.DescriptorPool            = imguiPool;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering       = true;

        initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &inColorFormat;

        ImGui_ImplSDL3_InitForVulkan(inWindow);
        ImGui_ImplVulkan_Init(&initInfo);
    }

    void ImGuiRenderer::Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, VkExtent2D inRenderExtent)
    {
        const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingInfo     renderInfo      = vkinit::rendering_info(inRenderExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(inCmd, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmd);

        vkCmdEndRendering(inCmd);
    }

    void ImGuiRenderer::CreateImGuiRenderPass(VkDevice inDevice, VkFormat inSwapchainImageFormat)
    {
        VkAttachmentDescription attachment = {};
        attachment.format                  = inSwapchainImageFormat;
        attachment.samples                 = VK_SAMPLE_COUNT_1_BIT;
        attachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_LOAD;
        attachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
        attachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment.initialLayout           = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        attachment.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        constexpr VkAttachmentReference color_attachment = {.attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        /*color_attachment.attachment            = 0;
        color_attachment.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;*/

        const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS, 
            .colorAttachmentCount = 1, 
            .pColorAttachments = &color_attachment};
        /*subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments    = &color_attachment;*/

        constexpr VkSubpassDependency dependency = {
        .srcSubpass          = VK_SUBPASS_EXTERNAL,
        .dstSubpass          = 0,
        .srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask       = 0, // or VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };

        VkRenderPassCreateInfo info = {};
        info.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        info.attachmentCount        = 1;
        info.pAttachments           = &attachment;
        info.subpassCount           = 1;
        info.pSubpasses             = &subpass;
        info.dependencyCount        = 1;
        info.pDependencies          = &dependency;

        if (vkCreateRenderPass(inDevice, &info, nullptr, &m_ImGuiRenderPass) != VK_SUCCESS)
            throw std::runtime_error("failed to create render pass!");
    }

    void ImGuiRenderer::CreateViewportRenderPass(VkDevice inDevie, const VkFormat inDepthFormat, const VkFormat inSwapImageFormat)
    {
        std::array<VkAttachmentDescription, 2> attachments = {};
        // Color attachment
        attachments[0].format         = inSwapImageFormat;
        attachments[0].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[0].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // Depth attachment
        attachments[1].format         = inDepthFormat;
        attachments[1].samples        = VK_SAMPLE_COUNT_1_BIT;
        attachments[1].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[1].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachments[1].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[1].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        constexpr VkAttachmentReference colorReference = {
        .attachment            = 0,
        .layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        constexpr VkAttachmentReference depthReference = {
        .attachment            = 1,
        .layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        };

        VkSubpassDescription subpassDescription    = {};
        subpassDescription.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpassDescription.colorAttachmentCount    = 1;
        subpassDescription.pColorAttachments       = &colorReference;
        subpassDescription.pDepthStencilAttachment = &depthReference;
        subpassDescription.inputAttachmentCount    = 0;
        subpassDescription.pInputAttachments       = nullptr;
        subpassDescription.preserveAttachmentCount = 0;
        subpassDescription.pPreserveAttachments    = nullptr;
        subpassDescription.pResolveAttachments     = nullptr;

        // Subpass dependencies for layout transitions
        std::array<VkSubpassDependency, 2> dependencies;

        dependencies[0].srcSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass      = 0;
        dependencies[0].srcStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[0].dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[0].srcAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[0].dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        dependencies[1].srcSubpass      = 0;
        dependencies[1].dstSubpass      = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependencies[1].dstStageMask    = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        dependencies[1].srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        dependencies[1].dstAccessMask   = VK_ACCESS_MEMORY_READ_BIT;
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments           = attachments.data();
        renderPassInfo.subpassCount           = 1;
        renderPassInfo.pSubpasses             = &subpassDescription;
        renderPassInfo.dependencyCount        = static_cast<uint32_t>(dependencies.size());
        renderPassInfo.pDependencies          = dependencies.data();

        if (vkCreateRenderPass(inDevie, &renderPassInfo, nullptr, &m_ViewportRenderPass) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create render pass!");
        }
    }
 
    void ImGuiRenderer::CreateImGuiFrameBuffers(VkDevice inDevie, const VkExtent2D &inExtent, const std::vector<VkImageView> &inSwapchainImageViews)
    {
        m_ImGuiFramebuffers.resize(inSwapchainImageViews.size());

        VkImageView             attachment[1];
        VkFramebufferCreateInfo info = {};
        info.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        //info.renderPass              = m_ImGuiRenderPass;
        info.attachmentCount         = 1;
        info.pAttachments            = attachment;
        info.width                   = inExtent.width;
        info.height                  = inExtent.height;
        info.layers                  = 1;

        for (uint32_t i = 0; i < inSwapchainImageViews.size(); i++)
        {
            attachment[0] = inSwapchainImageViews[i];
            if (vkCreateFramebuffer(inDevie, &info, nullptr, &m_ImGuiFramebuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void ImGuiRenderer::CreateViewportFrameBuffers(VkDevice inDevice, const VkExtent2D &inSwapchainExtent, VkImageView inDepthImageView)
    {
        m_ViewportFrameBuffers.resize(m_ViewportImageViews.size());
        for (size_t i = 0; i < m_ViewportImageViews.size(); i++)
        {
            const std::array<VkImageView, 2> attachments = {m_ViewportImageViews[i], inDepthImageView};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass      = m_ViewportRenderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments    = attachments.data();
            framebufferInfo.width           = inSwapchainExtent.width;
            framebufferInfo.height          = inSwapchainExtent.height;
            framebufferInfo.layers          = 1;

            if (vkCreateFramebuffer(inDevice, &framebufferInfo, nullptr, &m_ViewportFrameBuffers[i]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create framebuffer!");
            }
        }
    }

    void ImGuiRenderer::CreateCommandBuffers(VkDevice inDevice, const size_t inSizeSwapchainImgageViews)
    {
        m_ImGuiCommandBuffers.resize(inSizeSwapchainImgageViews);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool        = m_ImGuiCommandPool;
        allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)m_ImGuiCommandBuffers.size();

        if (vkAllocateCommandBuffers(inDevice, &allocInfo, m_ImGuiCommandBuffers.data()) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void ImGuiRenderer::CreateViewportCommandPool(VkDevice inDevie, const uint32_t inGraphicsQueueFamily)
    {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = inGraphicsQueueFamily;

        if (vkCreateCommandPool(inDevie, &poolInfo, nullptr, &m_ViewportCommandPool) != VK_SUCCESS)
        {
            throw std::runtime_error("failed to create command pool!");
        }
    }

   
}