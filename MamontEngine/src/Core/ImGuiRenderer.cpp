#include "ImGuiRenderer.h"
#include "ContextDevice.h"
#include "VkDestriptor.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>
#include "FrameData.h"

#include "VkInitializers.h"


namespace MamontEngine
{
    void ImGuiRenderer::Init(VkContextDevice &inContextDevice,
                             SDL_Window      *inWindow,
                             VkFormat         inColorFormat,
                             DeletionQueue   &inDeletionQueue,
                             VkDescriptorPool existingPool)
    {
        ImGui::CreateContext();

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = inContextDevice.Instance;
        initInfo.PhysicalDevice            = inContextDevice.ChosenGPU;
        initInfo.Device                    = inContextDevice.Device;
        initInfo.Queue                     = inContextDevice.GraphicsQueue;
        initInfo.DescriptorPool            = existingPool;
        initInfo.MinImageCount             = 3;
        initInfo.ImageCount                = 3;
        initInfo.MSAASamples               = VK_SAMPLE_COUNT_1_BIT;
        initInfo.UseDynamicRendering       = true;

        initInfo.PipelineRenderingCreateInfo                         = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
        initInfo.PipelineRenderingCreateInfo.colorAttachmentCount    = 1;
        auto ColorFormat                                             = inColorFormat;
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &ColorFormat;

        ImGui_ImplSDL3_InitForVulkan(inWindow);
        ImGui_ImplVulkan_Init(&initInfo);
        ImGui_ImplVulkan_CreateFontsTexture();
        fmt::println("InitImGuiRenderer");

        
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