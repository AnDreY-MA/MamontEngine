#pragma once

namespace MamontEngine
{
namespace PhysicalDevice
{
    void Init(VkPhysicalDevice inDevice);

	VkPhysicalDevice GetDevice();

	const VkPhysicalDeviceMemoryProperties &GetMemoryProperties();
}
}