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

        void *MappedData{nullptr};
    };

    struct MeshBuffer
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress{0};
    };
}