#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Core/Engine.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    void AllocatedBuffer::Create(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage)
    {
        const VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, .pNext = nullptr, .size = inAllocSize != 0 ? inAllocSize : 1, .usage = inUsage};

        const VmaAllocationCreateInfo vmaAllocInfo = {
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = inMemoryUsage,
        };

        VK_CHECK(vmaCreateBuffer(Allocator::GetAllocator(), &bufferInfo, &vmaAllocInfo, &Buffer, &Allocation, &Info));
        Size       = static_cast<uint64_t>(inAllocSize);
        Memory     = Allocation->GetMemory();
    }

    void AllocatedBuffer::Destroy()
    {
        if (Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(Allocator::GetAllocator(), Buffer, Allocation);
            Buffer = VK_NULL_HANDLE;
            Allocation = VK_NULL_HANDLE;
            Info       ={};
        }
    }

    VkResult AllocatedBuffer::Map(VkDeviceSize inSize, VkDeviceSize inOffset)
    {
        const VkDevice device = LogicalDevice::GetDevice();

        return vkMapMemory(device, Info.deviceMemory, inOffset, inSize, 0, &Info.pMappedData);
    }

    void *AllocatedBuffer::GetMappedData() const
    {
        return Info.pMappedData;
    }
}