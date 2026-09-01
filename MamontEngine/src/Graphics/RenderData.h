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
    constexpr uint32_t       SHADOW_MAP_FACE_IMAGE_SIZE{1024};
    constexpr uint32_t       SHADOW_FACE_NUM{6};
    constexpr uint32_t       MAX_POINT_LIGHT{8};

	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     MeshBuffer InMeshBuffer,

                     VkDescriptorSet InMaterial,
                     uint32_t InMaterialIndex,
                     AABB inBound,
                     glm::mat4       InTransform,
                     UID id)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex), MeshBuffer(InMeshBuffer)
            , MaterialDescriptorSet(InMaterial)
            , MaterialIndex(InMaterialIndex)
            , Bound(inBound)
            , Transform(InTransform)
            , Id(id)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        const MeshBuffer     MeshBuffer;

        VkDescriptorSet MaterialDescriptorSet;
        uint32_t        MaterialIndex{0};
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
        glm::vec3 Position;
        float     padding0;
        glm::vec3 Color;
        float     padding1;
        float     Radius;
        float     Attenuation;
        uint32_t  CastShadow{1};
        float     padding2;
    };
    static_assert(sizeof(PointLight) == 48, "PointLight size must be 48 bytes");

    struct LightData
    {
        glm::vec3 Splits;
        float     _pad0;
        glm::mat4 InverseViewMatrix;
        glm::vec3 LightDirection;
        float     _pad1;
        glm::vec3 Color;
        uint32_t  HasDirectionLight;

        PointLight PointLights[8];
        uint32_t   PointLightingCount;
        float      _pad2[3];           
    };
    static_assert(sizeof(LightData) == 512, "LightData size must be 512 bytes");

    struct ShadowCube
    {
        glm::mat4 FaceViewProjs[6];
    };
} // namespace MamontEngine
/*

*/