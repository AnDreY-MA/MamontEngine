#pragma once

#include "Graphics/Vulkan/Buffers/Buffer.h"

namespace MamontEngine
{
    struct Vertex;

    struct MeshBuffer
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress{0};
        
        void Create(std::span<uint32_t> inIndices, std::span<Vertex> inVertices);
    };
}
