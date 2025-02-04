#pragma once

namespace MamontEngine
{
    struct AllocatedBuffer
    {
        VkBuffer          Buffer;
        VmaAllocation     Allocation;
        VmaAllocationInfo Info;
    };

    struct GPUMeshBuffers
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress;
    };
}