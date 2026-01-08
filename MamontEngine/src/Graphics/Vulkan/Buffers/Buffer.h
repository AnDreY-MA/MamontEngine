#pragma once

namespace MamontEngine
{
    struct AllocatedBuffer
    {
        VkBuffer          Buffer{VK_NULL_HANDLE};
        VmaAllocation     Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;

        VkDescriptorBufferInfo DescriptorInfo;

        VkDeviceSize Offset{0};

        void Create(const size_t inAllocSize, const VkBufferUsageFlags inUsage, const VmaMemoryUsage inMemoryUsage);

        void Destroy();

        void Copy(const void *inSrc) const;

        void Copy(const void *inSrc, size_t size, size_t offset = 0) const;

        void *GetMappedData() const;

        bool IsValid() const noexcept
        {
            return Buffer != VK_NULL_HANDLE;
        }

        VkDescriptorBufferInfo GetDescriptorInfo() const
        {
            const VkDescriptorBufferInfo info{
                .buffer = Buffer, .offset = Offset, .range = Info.size
            };

            return info;
        }
    };

    AllocatedBuffer CreateStagingBuffer(const size_t inAllocationSize);
} // namespace MamontEngine

