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
        const VkDevice& device = LogicalDevice::GetDevice();

        vkDestroyPipelineLayout(device, Layout, nullptr);
        vkDestroyPipeline(device, Pipeline, nullptr);
    }
}