#pragma once

namespace MamontEngine::Utils
{
    uint32 FindMemoryType(const VkPhysicalDevice inPhysicalDevice, const uint32 inFilter, const VkMemoryPropertyFlags inProps)
    {
        // #TODO Move this to the Physical device abstraction once we create it
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(inPhysicalDevice, &memoryProperties);

        for (uint32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            // Check if this filter bit flag is set and it matches our memory properties
            if ((inFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & inProps) == inProps)
            {
                return i;
            }
        }

        fmt::println("Failed to find suitable memory type!");
        return 0;
    }
}