#pragma once 

#include "Graphics/Resources/Material.h"
#include "Math/AABB.h"
#include "Math/Transform.h"

namespace MamontEngine
{
    struct Vertex
    {
        alignas(16) glm::vec3 Position = glm::vec3(0.0f);
        alignas(16) glm::vec3 Normal   = glm::vec3(0.0f);
        alignas(8) glm::vec2 UV = glm::vec2{0.f};
        alignas(16) glm::vec4 Color = glm::vec4(1.f);
        alignas(16) glm::vec4 Tangent = glm::vec4(0.f);
    };

    struct GPUDrawPushConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(0.f)};
        VkDeviceAddress VertexBuffer{0};
        uint64_t        ObjectID{0};
        uint32_t        CascadeIndex{0};
    };

    struct PickingPushConstants
    {
        uint32_t ObjectID{0};
    };

    struct Primitive
    {
        uint32_t StartIndex{0};
        uint32_t Count{0};

        Material* Material;
    };

    struct NewMesh
    {
        std::vector<std::unique_ptr<Primitive>> Primitives;

        std::string Name;
    };

    struct Node
    {
        Node()  = default;
        ~Node() = default;

        Node* Parent;
        std::vector<Node*>        Children;
        std::shared_ptr<NewMesh> Mesh;

        glm::mat4 Matrix;
        glm::mat4 CurrentMatrix{glm::mat4(1.f)};
        Transform Transform;

        std::string Name;
    };
}
