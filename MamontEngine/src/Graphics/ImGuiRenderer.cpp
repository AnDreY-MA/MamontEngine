#include "ImGuiRenderer.h"
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include "Core/ContextDevice.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Utils/VkInitializers.h"
#include "imgui/imgui.h"
#include "Core/Engine.h"
#include "Core/ImGuiLayer.h"
#include "Utils/Profile.h"

namespace MamontEngine
{
    static ImGuiRenderer *s_ImGuiRenderer{nullptr};

    ImGuiRenderer &ImGuiRenderer::Get()
    {
        return *s_ImGuiRenderer;
    }
    ImGuiRenderer::ImGuiRenderer(const VkContextDevice             &inContextDevice,
                                 SDL_Window                        *inWindow,
                                 VkFormat                           inColorFormat,
                                 const std::shared_ptr<ImGuiLayer> &inLayer) 
        : m_ColorFormat(inColorFormat), m_Layer(inLayer)
    {
        const std::array<VkDescriptorPoolSize, 11> poolSizes = {VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_SAMPLER, 8},
                                                                VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 128},
                                                      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 128},
                                                      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 8},
                                                      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 8},
                                                      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 8},
                                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 32},
                                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 32},
                                                      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 16},
                                                      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 16},
                                                      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 8}};

        uint32_t maxSets = 0;
        for (const VkDescriptorPoolSize &ps : poolSizes)
        {
            maxSets += ps.descriptorCount;
        }

        const VkDescriptorPoolCreateInfo poolInfo = {.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                                                     .pNext         = nullptr,
                                                     .flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
                                                     .maxSets       = maxSets,
                                                     .poolSizeCount = (uint32_t)std::size(poolSizes),
                                                     .pPoolSizes    = poolSizes.data()};
        VkDevice                         device   = LogicalDevice::GetDevice();

        VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool));

        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance                  = inContextDevice.GetInstance();
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
        initInfo.PipelineRenderingCreateInfo.pColorAttachmentFormats = &inColorFormat;

        ImGui_ImplSDL3_InitForVulkan(inWindow);
        ImGui_ImplVulkan_Init(&initInfo);

        s_ImGuiRenderer = this;
        m_Initialized   = true;
    }

    ImGuiRenderer::~ImGuiRenderer()
    {
        VkDevice device = LogicalDevice::GetDevice();
        vkResetDescriptorPool(device, m_DescriptorPool, 0);
        vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
        m_Initialized = false;
    }

    bool ImGuiRenderer::IsInitialize() 
    {
        return s_ImGuiRenderer != nullptr;
    }

    void ImGuiRenderer::Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, const VkExtent2D &inRenderExtent)
    {
        PROFILE_ZONE("Render ImGui");


        VkCommandBufferInheritanceRenderingInfo inheritanceDynamicInfo{};
        inheritanceDynamicInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_RENDERING_INFO;
        inheritanceDynamicInfo.colorAttachmentCount    = 1;
        inheritanceDynamicInfo.pColorAttachmentFormats = &m_ColorFormat;
        inheritanceDynamicInfo.viewMask                = 0;
        inheritanceDynamicInfo.rasterizationSamples    = VK_SAMPLE_COUNT_1_BIT;

        const VkCommandBufferInheritanceInfo inheritanceInfo = {
                .sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO,
                .pNext       = &inheritanceDynamicInfo,
                .renderPass  = VK_NULL_HANDLE, 
                .subpass     = 0,              
                .framebuffer = VK_NULL_HANDLE, 
        };

        VkCommandBufferBeginInfo cmdSecondaryBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        cmdSecondaryBeginInfo.pInheritanceInfo         = &inheritanceInfo;

        {
            VK_CHECK(vkBeginCommandBuffer(inCmd, &cmdSecondaryBeginInfo));

            const VkRenderingAttachmentInfo colorAttachment = vkinit::attachment_info(inTargetImageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            const VkRenderingInfo           renderInfo      = vkinit::rendering_info(inRenderExtent, &colorAttachment, nullptr);

            vkCmdBeginRendering(inCmd, &renderInfo);
            m_Layer->ImGuiRender();


            const VkViewport viewport = {.x        = 0.0f,
                                         .y        = 0.0f,
                                         .width    = static_cast<float>(inRenderExtent.width),
                                         .height   = static_cast<float>(inRenderExtent.height),
                                         .minDepth = 0.0f,
                                         .maxDepth = 1.0f};

            const VkRect2D scissor = {.offset = {0, 0}, .extent = inRenderExtent};

            vkCmdSetViewport(inCmd, 0, 1, &viewport);
            vkCmdSetScissor(inCmd, 0, 1, &scissor);

            ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), inCmd);

            vkCmdEndRendering(inCmd);

            VK_CHECK(vkEndCommandBuffer(inCmd));
        }
    }

} // namespace MamontEngine

