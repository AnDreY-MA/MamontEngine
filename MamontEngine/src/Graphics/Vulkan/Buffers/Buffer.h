#pragma once

namespace MamontEngine
{
    struct AllocatedBuffer
    {
        VkBuffer          Buffer{VK_NULL_HANDLE};
        VmaAllocation     Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;
        VkDeviceMemory    Memory{VK_NULL_HANDLE};

        VkDeviceSize Size{0};

        VkDescriptorBufferInfo DescriptorInfo;

        void Create(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage);

        void Destroy();

        void *GetMappedData() const;

        bool IsValid() const noexcept
        {
            return Buffer != VK_NULL_HANDLE;
        }
    };

    AllocatedBuffer CreateStagingBuffer(const size_t inAllocationSize);
}