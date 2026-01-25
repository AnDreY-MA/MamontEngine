#pragma once

#include "Graphics/Resources/Models/Mesh.h"
#include <bitset>
#include "Core/UID.h"
#include "Graphics/Vulkan/Buffers/MeshBuffer.h"

namespace MamontEngine
{
    constexpr const size_t   CASCADECOUNT = 5;
    constexpr const uint32_t SHADOWMAP_DIMENSION{4096};

	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     MeshBuffer InMeshBuffer,

                     VkDescriptorSet InMaterial,
                     AABB inBound,
                     glm::mat4       InTransform,
                     UID id)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex), MeshBuffer(InMeshBuffer)
            , MaterialDescriptorSet(InMaterial)
            , Bound(inBound)
            , Transform(InTransform)
            , Id(id)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        const MeshBuffer     MeshBuffer;

        VkDescriptorSet MaterialDescriptorSet;
		//const Material *Material;
        AABB            Bound;

		glm::mat4 Transform;

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
        glm::vec3              Color{glm::vec3(1.0)};
        bool                 IsActive{false};
    };
}