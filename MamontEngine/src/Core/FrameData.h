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

    struct FrameData
    {
        VkSemaphore SwapchainSemaphore;
        VkSemaphore RenderSemaphore;

        VkFence RenderFence;

        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;

        DeletionQueue               Deleteions;
        DescriptorAllocatorGrowable FrameDescriptors;
    };
}