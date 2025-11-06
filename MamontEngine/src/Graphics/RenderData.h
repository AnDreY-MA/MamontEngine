#pragma once

#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Mesh.h"
#include <bitset>
#include "Core/UID.h"

namespace MamontEngine
{
    constexpr const size_t   CASCADECOUNT = 5;
    constexpr const uint32_t SHADOWMAP_DIMENSION{4096};

	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     const VkBuffer InIndexBuffer,
                     const VkBuffer  inVertexBuffer,

                     const GLTFMaterial *InMaterial,
                     AABB inBound,
                     glm::mat4       InTransform,
                     VkDeviceAddress InVertexBufferAdders, UID id)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex)
            , IndexBuffer(InIndexBuffer)
            , VertexBuffer(inVertexBuffer)
            , Material(InMaterial)
            , Bound(inBound)
            , Transform(InTransform)
            , VertexBufferAddress(InVertexBufferAdders)
            , Id(id)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        const VkBuffer IndexBuffer;
        const VkBuffer VertexBuffer;

		const GLTFMaterial *Material;
        AABB            Bound;

		glm::mat4 Transform;
        VkDeviceAddress VertexBufferAddress;

        UID Id;
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

    struct Cascade
    {
        VkImageView View;
        float       SplitDepth{0.f};
        glm::mat4   ViewProjectMatrix{glm::mat4(0.f)};
    };

    struct CascadeData
    {
        std::array<float, 4> Splits{};
        glm::mat4            InverseViewMatrix{glm::mat4(0.f)};
        glm::vec3 LightDirection{glm::vec3()};
        float                _pad;
        int32_t              Color{1};
    };
}