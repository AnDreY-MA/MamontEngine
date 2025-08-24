#include "PhysicalDevice.h"

namespace MamontEngine
{
    PhysicalDevice::PhysicalDevice(VkPhysicalDevice inDevice)
        : m_Device(inDevice)
    {
        vkGetPhysicalDeviceMemoryProperties(m_Device, &m_MemoryProperties);
    }
}