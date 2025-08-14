#pragma once

#include "VkDestriptor.h"
#include "pch.h"
#include "Graphics/Vulkan/Image.h"
#include "Graphics/Vulkan/VkMaterial.h"
#include "Graphics/Mesh.h"


namespace MamontEngine
{
	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     VkBuffer InIndexBuffer,

                     MaterialInstance* InMaterial,
                     Bounds inBound,
                     glm::mat4       InTransform,
                     VkDeviceAddress InVertexBufferAdders)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex)
            , IndexBuffer(InIndexBuffer)
            , Material(InMaterial)
            , Bound(inBound)
            , Transform(InTransform)
            , VertexBufferAddress(InVertexBufferAdders)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        VkBuffer IndexBuffer;

		MaterialInstance *Material;
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

	struct GLTFMetallic_Roughness
    {
        struct MaterialConstants
        {
            glm::vec4 ColorFactors;
            glm::vec4 Metal_rough_factors;
            glm::vec4 Extra[14];
        };

        struct MaterialResources
        {
            AllocatedImage ColorImage;
            VkSampler      ColorSampler;
            AllocatedImage MetalRoughImage;
            VkSampler      MetalRoughSampler;
            VkBuffer       DataBuffer;
            uint32_t       DataBufferOffset;
        };
    };

}