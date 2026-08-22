#include "Graphics/Vulkan/Pipelines/PipelineData.h"
#include "Graphics/Devices/LogicalDevice.h"

namespace MamontEngine
{
    PipelineData::PipelineData(VkPipeline inPipeline, VkPipelineLayout inLayout, VkPipelineCache inCache)
        : Pipeline(inPipeline), Layout(inLayout), Cache(inCache)
    {

    }

    PipelineData::~PipelineData()
    {
        const VkDevice& device = LogicalDevice::GetDevice();

        vkDestroyPipelineLayout(device, Layout, nullptr);
        vkDestroyPipeline(device, Pipeline, nullptr);
        
        if (Cache != VK_NULL_HANDLE)
        {
            vkDestroyPipelineCache(device, Cache, nullptr);
        }
    }
}