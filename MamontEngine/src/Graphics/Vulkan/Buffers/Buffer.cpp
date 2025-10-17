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

    void *AllocatedBuffer::GetMappedData() const
    {
        return Info.pMappedData;
    }

    AllocatedBuffer CreateStagingBuffer(const size_t inAllocationSize)
    {
        AllocatedBuffer newBuffer;
        newBuffer.Create(inAllocationSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);
        return newBuffer;
    }
}