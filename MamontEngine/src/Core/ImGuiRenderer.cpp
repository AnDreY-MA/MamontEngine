#include "ImGuiRenderer.h"
#include "ContextDevice.h"
#include "VkDestriptor.h"
#include "imgui/imgui.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include "FrameData.h"

#include "VkInitializers.h"

namespace MamontEngine
{
    void ImGuiRenderer::Init(VkContextDevice &inContextDevice, SDL_Window *inWindow, VkFormat inColorFormat, DeletionQueue &inDeletionQueue)
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
        poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets                    = 1000;
        poolInfo.poolSizeCount              = (uint32_t)std::size(poolSizes);
        poolInfo.pPoolSizes                 = poolSizes;

        VkDescriptorPool imguiPool;
        VK_CHECK_MESSAGE(vkCreateDescriptorPool(inContextDevice.Device, &poolInfo, nullptr, &imguiPool), "CreateDescPool");

        ImGui::CreateContext();

        ImGui_ImplSDL3_InitForVulkan(inWindow);

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = inContextDevice.Instance;
        initInfo.PhysicalDevice            = inContextDevice.ChosenGPU;
        initInfo.Device                    = inContextDevice.Device;
        initInfo.Queue                     = inContextDevice.GraphicsQueue;
        initInfo.DescriptorPool            = imguiPool;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering       = true;

        initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        auto ColorFormat                                             = inColorFormat;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &ColorFormat;

        ImGui_ImplVulkan_Init(&initInfo);

        ImGui_ImplVulkan_CreateFontsTexture();

        inDeletionQueue.PushFunction(
                [=]()
                {
                    ImGui_ImplVulkan_Shutdown();
                    vkDestroyDescriptorPool(inContextDevice.Device, imguiPool, nullptr);
                });
    }

    void ImGuiRenderer::Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, VkExtent2D inRenderExtent)
    {
        VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
        const VkRenderingInfo     renderInfo      = vkinit::rendering_info(inRenderExtent, &colorAttachment, nullptr);

        vkCmdBeginRendering(inCmd, &renderInfo);

        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmd);

        vkCmdEndRendering(inCmd);
    }
}