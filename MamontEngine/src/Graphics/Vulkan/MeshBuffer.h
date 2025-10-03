#pragma once

#include "Graphics/Vulkan/Buffers/Buffer.h"

namespace MamontEngine
{
    struct MeshBuffer
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress{0};
    };
}