#pragma once

#include "Core/VkDestriptor.h"

namespace MamontEngine
{
    struct DeletionQueue
    {
    public:
        void PushFunction(std::function<void()> &&inFunction)
        {
            deletors.push_back(inFunction);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }

            deletors.clear();
        }

    private:
        std::deque<std::function<void()>> deletors;
    };

    struct ViewportData
    {
        VkRenderPass                 ViewportRenderPass;
        VkPipeline                   ViewportPipeline;
        VkCommandPool                ViewportCommandPool;
        std::vector<VkFramebuffer>   ViewportFramebuffers;
        std::vector<VkCommandBuffer> ViewportCommandBuffers;
    };

    struct FrameData
    {
        VkSemaphore SwapchainSemaphore;
        VkSemaphore RenderSemaphore;

        VkFence RenderFence;

        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;

        ViewportData Viewport;

        DeletionQueue               Deleteions;
        DescriptorAllocatorGrowable FrameDescriptors;
    };
}