#include "PhysicalDevice.h"

namespace MamontEngine
{
namespace PhysicalDevice
{
    VkPhysicalDevice                 g_Device;
    VkPhysicalDeviceMemoryProperties g_MemoryProperties;

    void Init(VkPhysicalDevice inDevice)
    {
        g_Device = inDevice;
        vkGetPhysicalDeviceMemoryProperties(g_Device, &g_MemoryProperties);
    }

    VkPhysicalDevice GetDevice()
    {
        return g_Device;
    }

    const VkPhysicalDeviceMemoryProperties &GetMemoryProperties()
    {
        return g_MemoryProperties;
    }
}
    
}