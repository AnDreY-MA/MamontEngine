#pragma once

#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Mesh.h"
#include <bitset>

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

		const GLTFMaterial *Material;
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

    struct Cascade
    {
        VkImageView View;
        float       SplitDepth{0.f};
        glm::mat4   ViewProjectMatrix{glm::mat4(0.f)};
    };

    struct ShadowCascadeUBO
    {
        static constexpr const int kDebugCascadeBit = 0;
        static constexpr const int kPCFBit          = 0;
        std::bitset<64>            SettingBits{0};
        glm::vec2                  Pad;
        float                      MinBias{0.001};
        float                      MaxBias{0.005};
        float                      PcfScale{0.75};
        float                      Pad2;
        glm::vec4                  PlaneDistances;
    };

    struct ShadowCascadeMatrices
    {
        std::array<glm::mat4, 5> LightSpaceMatrices;
    };

}