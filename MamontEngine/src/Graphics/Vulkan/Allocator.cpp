#include <Graphics/Vulkan/Allocator.h>

namespace MamontEngine
{
namespace Allocator
{
    VmaAllocator g_memoryAllocator = VK_NULL_HANDLE;

    VmaAllocator &GetAllocator()
    {
        return g_memoryAllocator;
    }

    void Init(const VmaAllocatorCreateInfo& create_info)
    {
        if (g_memoryAllocator == VK_NULL_HANDLE)
        {
            VK_CHECK(vmaCreateAllocator(&create_info, &g_memoryAllocator));
        }
    }

    void Destroy()
    {
        if (g_memoryAllocator != VK_NULL_HANDLE)
        {
            VmaTotalStatistics stats;
            vmaCalculateStatistics(g_memoryAllocator, &stats);
            fmt::println("Total device memory leaked: {} bytes", stats.total.statistics.allocationBytes);
            vmaDestroyAllocator(g_memoryAllocator);
            g_memoryAllocator = VK_NULL_HANDLE;
        }
    }
}
}