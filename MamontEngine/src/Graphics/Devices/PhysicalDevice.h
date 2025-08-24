#pragma once

namespace MamontEngine
{
	class PhysicalDevice
	{
    public:
        explicit PhysicalDevice(VkPhysicalDevice inDevice);
        
		~PhysicalDevice() = default;

		VkPhysicalDevice GetDevice() const
		{
        	return m_Device;
		}

		const VkPhysicalDeviceMemoryProperties& GetMemoryProperties() const
		{
			return m_MemoryProperties;
		}

	private:
        VkPhysicalDevice                 m_Device;
        VkPhysicalDeviceMemoryProperties m_MemoryProperties;
	};
}
