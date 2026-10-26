#pragma once 

#include "Graphics/Resources/Materials/Material.h"
#include "Math/AABB.h"
#include "Math/Transform.h"

namespace MamontEngine
{
    enum TransformStatus : uint8_t
    {
        Clean = 0,
        LocalDirty = 1 << 0,
        WorldDirty = 1 << 1,
    };

    struct MyData
    {
        glm::vec3 p;
        float     r;
    };

    struct Vertex
    {
        alignas(16) glm::vec3 Position = glm::vec3(0.0f);
        alignas(16) glm::vec3 Normal   = glm::vec3(0.0f);
        alignas(8) glm::vec2 UV       = glm::vec2{0.f};
        alignas(16) glm::vec4 Color    = glm::vec4(1.f);
        alignas(16) glm::vec4 Tangent  = glm::vec4(0.f);
    };

    struct GPUDrawPushConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(1.f)};
        VkDeviceAddress VertexBuffer{0};
        VkDeviceAddress MaterialBuffer{0};
        uint64_t        ObjectID{0};
        uint32_t        CascadeIndex{0};
        uint32_t        MaterialIndex{0};
    };

    struct SphereBounds
    {
        glm::vec3 Origin{glm::vec3(0.f)};
        float     Radius{0.f};
        glm::vec3 Extents{glm::vec3(0.0f)};
    };

    struct Primitive
    {
        uint32_t StartIndex{0};
        uint32_t Count{0};

        uint32_t MaterialIndex;

        SphereBounds Bounds;

        AABB LocalBounds;
    };

    struct Mesh
    {
        std::vector<std::unique_ptr<Primitive>> Primitives;

        std::string Name;
    };

    static constexpr uint32_t INVALID_NODE_INDEX = 0xFFFFFFFF;

    struct Node
    {
        Node()  = default;
        ~Node() = default;

        uint32_t                    ParentIndex{INVALID_NODE_INDEX};
        std::vector<uint32_t>       Children;
        uint32_t                    MeshIndex{0};

        glm::mat4                   Matrix{glm::mat4(1.f)};
        glm::mat4                   CurrentMatrix{glm::mat4(1.f)};
        Transform                   Transform;

        std::string                 Name;

        uint8_t Status = TransformStatus::Clean;

        void MarkDirty()
        {
            Status |= TransformStatus::LocalDirty | TransformStatus::WorldDirty;
        }
    };
}
