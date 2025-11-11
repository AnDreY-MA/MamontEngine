#include "Graphics/Vulkan/Buffers/Buffer.h"
#include <cstring>
#include "Graphics/Vulkan/Allocator.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace MamontEngine
{
    void AllocatedBuffer::Create(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage)
    {
        const VkBufferCreateInfo bufferInfo = {
                .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 
                .pNext = nullptr, 
                .size = inAllocSize != 0 ? inAllocSize : 1, 
                .usage = inUsage
        };

        const VmaAllocationCreateInfo vmaAllocInfo = {
                .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                .usage = inMemoryUsage,
        };

        VK_CHECK(vmaCreateBuffer(Allocator::GetAllocator(), &bufferInfo, &vmaAllocInfo, &Buffer, &Allocation, &Info));
    }

    void AllocatedBuffer::Destroy()
    {
        if (Buffer != VK_NULL_HANDLE)
        {
            vmaDestroyBuffer(Allocator::GetAllocator(), Buffer, Allocation);
            Buffer     = VK_NULL_HANDLE;
            Allocation = VK_NULL_HANDLE;
            Info       = {};
        }
    }

    void AllocatedBuffer::Copy(const void *inSrc) const
    {
        memcpy(Info.pMappedData, inSrc, Info.size);
    }

    void AllocatedBuffer::Copy(const void *inSrc, size_t size, size_t offset) const
    {
        void *dst = static_cast<char *>(Info.pMappedData) + offset;
        memcpy(dst, inSrc, size);
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
} // namespace MamontEngine

