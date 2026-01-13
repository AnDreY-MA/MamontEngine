#pragma once

namespace MamontEngine::Utils
{
    VkFormat FindSupportedFormat(VkPhysicalDevice device, const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

    VkFormat FindDepthFormat(VkPhysicalDevice device);

    uint32_t GetMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound = nullptr);

    template <typename T>
    inline T AlignUp(T size, T alignment)
    {
        return (size + (alignment - 1)) & ~(alignment - 1);
    }
}