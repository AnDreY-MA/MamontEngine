#pragma once

namespace MamontEngine
{
    struct PipelineData
    {
        PipelineData(VkPipeline inPipeline, VkPipelineLayout inLayout)
            : Pipeline(inPipeline), Layout(inLayout)
        {

        }

        VkPipeline       Pipeline{VK_NULL_HANDLE};
        VkPipelineLayout Layout{VK_NULL_HANDLE};
    };
}