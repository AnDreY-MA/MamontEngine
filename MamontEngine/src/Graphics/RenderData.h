#pragma once

#include "Graphics/Resources/Models/Mesh.h"
#include <bitset>
#include "Core/UID.h"
#include "Graphics/Vulkan/Buffers/MeshBuffer.h"

namespace MamontEngine
{
    constexpr const size_t   CASCADECOUNT = 4;
    constexpr const uint32_t SHADOWMAP_DIMENSION{4096};

    constexpr size_t         FRAME_OVERLAP = 3;
    constexpr uint32_t       shadowMapFaceImageSize{1024};
    constexpr uint32_t       SHADOW_FACE_NUM{6};
    constexpr uint32_t       MAX_POINT_LIGHT{8};

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

    struct PointLight
    {
        glm::vec3 Position;    // offset 0
        float     padding0;    // offset 12
        glm::vec3 Color;       // offset 16
        float     padding1;    // offset 28
        float     Radius;      // offset 32
        float     Attenuation; // offset 36
        uint32_t  CastShadow{1}; // offset 40 (bool в std140 = 4 байта)
        float     padding2;    // offset 44, добиваем до 48
    };
    static_assert(sizeof(PointLight) == 48, "PointLight size must be 48 bytes");

    struct LightData
    {
        glm::vec3 Splits;            // offset 0 (vec3)
        float     _pad0;             // offset 12 (выравнивание перед mat4)
        glm::mat4 InverseViewMatrix; // offset 16 (mat4, 64 байта)
        glm::vec3 LightDirection;    // offset 80
        float     _pad1;             // offset 92
        glm::vec3 Color;             // offset 96
        uint32_t  HasDirectionLight; // offset 108 (bool = 4 байта)
        // далее массив PointLight с выравниванием 16, начнётся со 112
        PointLight PointLights[8];     // offset 112, размер 8*48=384
        uint32_t   PointLightingCount; // offset 496
        float      _pad2[3];           // offset 500, добиваем до 512
    };
    static_assert(sizeof(LightData) == 512, "LightData size must be 512 bytes");

    struct ShadowCube
    {
        glm::mat4 FaceViewProjs[6];
    };
} // namespace MamontEngine
/*

*/