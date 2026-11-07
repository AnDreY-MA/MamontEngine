#pragma once

#include "Graphics/Resources/Models/Mesh.h"
#include <bitset>
#include "Core/UID.h"
#include "Graphics/Vulkan/Buffers/MeshBuffer.h"

namespace MamontEngine
{
    constexpr const size_t   CASCADECOUNT = 4;
    constexpr const uint32_t SHADOWMAP_DIMENSION{2048};

    constexpr size_t         FRAME_OVERLAP = 3;
    constexpr uint32_t       SHADOW_MAP_FACE_IMAGE_SIZE{1024};
    constexpr uint32_t       SHADOW_FACE_NUM{6};
    constexpr uint32_t       MAX_POINT_LIGHT{8};

	struct RenderObject
	{
        VkBuffer        IndexBuffer;
        VkDeviceAddress VertexAddress{0};

        uint32_t IndexCount;
        uint32_t FirstIndex;

        uint32_t        MaterialIndex{0};
        SphereBounds SphereBound;

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

    struct GPUDrawPushConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(1.f)};
        VkDeviceAddress SceneBuffer{0};
        VkDeviceAddress VertexBuffer{0};
        VkDeviceAddress MaterialBuffer{0};
        uint32_t        CascadeIndex{0};
        uint32_t        MaterialIndex{0};
    };

    struct alignas(16) Cascade
    {
        glm::mat4 ViewProjectMatrix{glm::mat4(1.f)};
        float     DepthSplit{0.f};
    };

    struct ShadowCascades
    { 
        std::array<glm::mat4, CASCADECOUNT> ViewProjectMatrixs{};
        std::array<float, CASCADECOUNT>     DepthSpilts{};
    };

    struct alignas(16) PointLight
    {
        glm::vec3 Position;
        float     _pad0;
        glm::vec3 Color;
        float     _pad1;
        float     Radius;
        float     Attenuation;
        uint32_t  CastShadow{1};  
        float     _pad2;   
    };

    struct alignas(16) DirectionLight
    {
        glm::vec3   Color;   
        float       pad0;                   
        glm::vec3   Direction;
        uint32_t    CastShadow{1};
    };

    struct alignas(16) LightData
    {
        DirectionLight  DirectLight;           
        uint32_t        HasDirectionLight;  

        PointLight      PointLights[MAX_POINT_LIGHT];
        uint32_t        PointLightingCount;

        std::array<Cascade, CASCADECOUNT> Cascade;
    };

} // namespace MamontEngine