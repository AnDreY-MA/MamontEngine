#include "Graphics/Vulkan/Buffers/MeshBuffer.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Core/Engine.h"
#include "Graphics/Vulkan/ImmediateContext.h"

namespace MamontEngine
{
    void MeshBuffer::Create(std::span<uint32_t> inIndices, std::span<Vertex> inVertices)
    {
        const size_t vertexBufferSize{inVertices.size() * sizeof(Vertex)};
        const size_t indexBufferSize{inIndices.size() * sizeof(uint32_t)};

        VertexBuffer.Create(vertexBufferSize,
                                       VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                       VMA_MEMORY_USAGE_GPU_ONLY);

        const VkBufferDeviceAddressInfo deviceAddressInfo = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, 
            .buffer = VertexBuffer.Buffer
        };
        VertexBufferAddress                    = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);

        IndexBuffer.Create(indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VMA_MEMORY_USAGE_GPU_ONLY);

        AllocatedBuffer staging = CreateStagingBuffer(vertexBufferSize + indexBufferSize);
        //staging.Create(vertexBufferSize + indexBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_MEMORY_USAGE_CPU_ONLY);

        /*VmaAllocationInfo allocInfo;
        vmaGetAllocationInfo(Allocator::GetAllocator(), staging.Allocation, &allocInfo);*/
        void *data = staging.Info.pMappedData;
        // void *data = staging.Allocation->GetMappedData;

        memcpy(data, inVertices.data(), vertexBufferSize);
        memcpy((char *)data + vertexBufferSize, inIndices.data(), indexBufferSize);

        const auto &ContextDevice = MEngine::Get().GetContextDevice();

        ImmediateContext::ImmediateSubmit(
                [&](VkCommandBuffer cmd)
                {
                    const VkBufferCopy vertexCopy = {.srcOffset = 0, .dstOffset = 0, .size = vertexBufferSize};

                    vkCmdCopyBuffer(cmd, staging.Buffer, VertexBuffer.Buffer, 1, &vertexCopy);

                    const VkBufferCopy indexCopy = {.srcOffset = vertexBufferSize, .dstOffset = 0, .size = indexBufferSize};

                    vkCmdCopyBuffer(cmd, staging.Buffer, IndexBuffer.Buffer, 1, &indexCopy);
                });

        staging.Destroy();
    }
}
