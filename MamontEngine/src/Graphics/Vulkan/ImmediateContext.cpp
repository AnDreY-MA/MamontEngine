#include "Graphics/Vulkan/ImmediateContext.h"
#include "Utils/VkInitializers.h"
#include "Graphics/Devices/LogicalDevice.h"

namespace MamontEngine
{
namespace ImmediateContext
{
    struct ImmediateContextData
    {
        VkCommandPool   CommandPool{VK_NULL_HANDLE};
        VkCommandBuffer CommandBuffer{VK_NULL_HANDLE};
        VkFence         Fence{VK_NULL_HANDLE};
        VkQueue         GraphicsQueue{VK_NULL_HANDLE};

    } s_ImmediateContext;

    void InitImmediateContext(VkQueue graphicQueue, uint32_t queueFamilyIndex)
    {
        const VkDevice                device          = LogicalDevice::GetDevice();

        const VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(queueFamilyIndex, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, nullptr, &s_ImmediateContext.CommandPool));

        const VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(s_ImmediateContext.CommandPool, 1);

        VK_CHECK(vkAllocateCommandBuffers(device, &cmdAllocInfo, &s_ImmediateContext.CommandBuffer));
        
        const VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);
        
        VK_CHECK(vkCreateFence(device, &fenceCreateInfo, nullptr, &s_ImmediateContext.Fence));

        s_ImmediateContext.GraphicsQueue = graphicQueue;
    }

    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)>&& inFunction)
    {
        fmt::println("ImmediateSubmit");

        const VkDevice  device = LogicalDevice::GetDevice();
        VkCommandBuffer cmd    = s_ImmediateContext.CommandBuffer;

        if (vkWaitForFences(device, 1, &s_ImmediateContext.Fence, VK_TRUE, UINT64_MAX) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to wait for immediate fence");
        }

        VK_CHECK(vkResetFences(device, 1, &s_ImmediateContext.Fence));

        VK_CHECK(vkResetCommandBuffer(cmd, 0));

        const VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

        inFunction(cmd);

        VK_CHECK(vkEndCommandBuffer(cmd));

        const VkCommandBufferSubmitInfo cmdInfo = vkinit::command_buffer_submit_info(cmd);
        const VkSubmitInfo2             submit  = vkinit::submit_info(&cmdInfo, nullptr, nullptr);

        VK_CHECK(vkQueueSubmit2(s_ImmediateContext.GraphicsQueue, 1, &submit, s_ImmediateContext.Fence));

        VK_CHECK(vkWaitForFences(device, 1, &s_ImmediateContext.Fence, VK_TRUE, UINT64_MAX));
    }

    void DestroyImmediateContext()
    {
        const VkDevice device = LogicalDevice::GetDevice();

        vkDestroyFence(device, s_ImmediateContext.Fence, nullptr);
        vkDestroyCommandPool(device, s_ImmediateContext.CommandPool, nullptr);
    }
}
} // namespace MamontEngine