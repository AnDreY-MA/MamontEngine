#pragma once

namespace MamontEngine::Utils
{
    VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat FindDepthFormat(VkPhysicalDevice device);
}