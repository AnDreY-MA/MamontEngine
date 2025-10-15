#pragma once

#include "Core/VkDestriptor.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "tracy/TracyVulkan.hpp"

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
        VkSemaphore SwapchainSemaphore{VK_NULL_HANDLE};
        //VkSemaphore RenderSemaphore{VK_NULL_HANDLE};

        VkFence RenderFence{VK_NULL_HANDLE};

        VkCommandPool   CommandPool{VK_NULL_HANDLE};
        VkCommandBuffer MainCommandBuffer{VK_NULL_HANDLE};

        DeletionQueue               Deleteions;
        DescriptorAllocatorGrowable FrameDescriptors;
        VkDescriptorSet             GlobalDescriptor;
        
        AllocatedBuffer SceneDataBuffer;
        AllocatedBuffer ShadowmMatrixBuffer;
        AllocatedBuffer ShadowUBOBuffer;
        TracyVkCtx TracyContext = nullptr;

    };
}