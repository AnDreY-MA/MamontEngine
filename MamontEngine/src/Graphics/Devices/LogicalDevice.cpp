#include "Graphics/Devices/LogicalDevice.h"

namespace MamontEngine
{
namespace LogicalDevice
{
    VkDevice g_Device{VK_NULL_HANDLE};

    VkDevice& GetDevice()
    {
        return g_Device;
    }
  
    void InitDevice(VkDevice inDevice)
    {
        g_Device = inDevice;
    }

    void DestroyDevice()
    {
        fmt::println("LOGICAL DEVICE: Destroy");
        vkDestroyDevice(g_Device, nullptr);
    }
}
}