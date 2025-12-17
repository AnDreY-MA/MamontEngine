#include "Graphics/Vulkan/Pipelines/PipelineData.h"
#include "Graphics/Devices/LogicalDevice.h"

namespace MamontEngine
{
    PipelineData::PipelineData(VkPipeline inPipeline, VkPipelineLayout inLayout)
        : Pipeline(inPipeline), Layout(inLayout)
    {

    }

    PipelineData::~PipelineData()
    {
        const VkDevice device = LogicalDevice::GetDevice();
        vkDeviceWaitIdle(device);

        if (Layout != VK_NULL_HANDLE)
        {
            vkDestroyPipelineLayout(device, Layout, nullptr);
        }
        if (Pipeline != VK_NULL_HANDLE)
        {
            vkDestroyPipeline(device, Pipeline, nullptr);
        }
    }
}