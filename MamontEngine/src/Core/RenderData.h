#pragma once

#include "VkDestriptor.h"
#include "pch.h"
#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Mesh.h"

namespace MamontEngine
{
	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     const VkBuffer InIndexBuffer,
                     const VkBuffer  inVertexBuffer,

                     const MaterialInstance* InMaterial,
                     Bounds inBound,
                     glm::mat4       InTransform,
                     VkDeviceAddress InVertexBufferAdders)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex)
            , IndexBuffer(InIndexBuffer)
            , VertexBuffer(inVertexBuffer)
            , Material(InMaterial)
            , Bound(inBound)
            , Transform(InTransform)
            , VertexBufferAddress(InVertexBufferAdders)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        const VkBuffer IndexBuffer;
        const VkBuffer VertexBuffer;

		const MaterialInstance *Material;
        Bounds            Bound;

		glm::mat4 Transform;
        VkDeviceAddress VertexBufferAddress;
	};

    struct DrawContext
    {
        void Clear()
        {
            OpaqueSurfaces.clear();
            TransparentSurfaces.clear();
        }

        std::vector<RenderObject> OpaqueSurfaces;
        std::vector<RenderObject> TransparentSurfaces;
    };

}