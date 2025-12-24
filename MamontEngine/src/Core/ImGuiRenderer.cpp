#include "ImGuiRenderer.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "ContextDevice.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Utils/VkInitializers.h"
#include "imgui/imgui.h"

#include "Utils/Profile.h"

namespace MamontEngine
{
    ImGuiRenderer::ImGuiRenderer(const VkContextDevice &inContextDevice, SDL_Window *inWindow, VkFormat inColorFormat)
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
        VkDevice                         device   = LogicalDevice::GetDevice();

        VK_CHECK_MESSAGE(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool), "CreateDescPool");
        // outPoolResult = imguiPool;

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = inContextDevice.Instance;
        initInfo.PhysicalDevice            = PhysicalDevice::GetDevice();
        initInfo.Device                    = device;
        initInfo.QueueFamily               = inContextDevice.GetGraphicsQueueFamily();
        initInfo.Queue                     = inContextDevice.GetGraphicsQueue();
        initInfo.DescriptorPool            = m_DescriptorPool;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering       = true;

        initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO, .pNext = nullptr};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &inColorFormat;

        ImGui_ImplSDL3_InitForVulkan(inWindow);
        ImGui_ImplVulkan_Init(&initInfo);

        //vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    }

    ImGuiRenderer::~ImGuiRenderer()
    {
        VkDevice device = LogicalDevice::GetDevice();
        vkResetDescriptorPool(device, m_DescriptorPool, 0);
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
    }

    void ImGuiRenderer::Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, const VkExtent2D &inRenderExtent)
    {
        PROFILE_ZONE("ImGuiRenderer::Draw");

        const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingInfo           renderInfo      = vkinit::rendering_info(inRenderExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(inCmd, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmd);

        vkCmdEndRendering(inCmd);
    }

} // namespace MamontEngine

