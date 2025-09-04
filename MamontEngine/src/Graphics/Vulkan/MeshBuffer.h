#pragma once

namespace MamontEngine
{
    struct AllocatedBuffer
    {
        VkBuffer          Buffer{VK_NULL_HANDLE};
        VmaAllocation     Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;
    };

    struct MeshBuffer
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress{0};
    };
}