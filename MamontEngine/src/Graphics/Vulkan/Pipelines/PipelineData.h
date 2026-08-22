#pragma once

namespace MamontEngine
{
    struct PipelineData : public NonCopyable
    {
        PipelineData(VkPipeline inPipeline, VkPipelineLayout inLayout, VkPipelineCache inCache = VK_NULL_HANDLE);
        ~PipelineData();

        VkPipeline       Pipeline{VK_NULL_HANDLE};
        VkPipelineLayout Layout{VK_NULL_HANDLE};
        VkPipelineCache  Cache{VK_NULL_HANDLE};

        bool operator==(const PipelineData& other) const
        {
            return Pipeline == other.Pipeline && Layout == other.Layout;
        }

        bool operator!=(const PipelineData& other) const
        {
            return !(*this == other);
        }
    };
}