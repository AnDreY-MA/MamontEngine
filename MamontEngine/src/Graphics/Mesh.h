#pragma once 

#include "Graphics/Vulkan/MeshBuffer.h"
#include "Graphics/Vulkan/Materials/Material.h"
#include "Graphics/Vulkan/Image.h"
#include "Core/VkDestriptor.h"

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

    struct Bounds
    {
        glm::vec3 Origin  = glm::vec3(0.0f);
        glm::vec3 Extents = glm::vec3(0.0f);
        float     SpherRadius{0.0f};
    };

    struct GPUDrawPushConstants
    {
        glm::mat4       WorldMatrix{glm::mat4(0.f)};
        VkDeviceAddress VertexBuffer{0};
        uint32_t        CascadeIndex{0};
    };

    struct Primitive
    {
        uint32_t StartIndex{0};
        uint32_t Count{0};

        Bounds Bound;

        std::shared_ptr<GLTFMaterial> Material;
    };

    struct NewMesh
    {
        std::vector<std::unique_ptr<Primitive>> Primitives;
    };

    struct Node
    {
        std::weak_ptr<Node> Parent;
        std::vector<std::shared_ptr<Node>> Children;
        std::shared_ptr<NewMesh> Mesh;

        glm::mat4 Matrix;
        glm::mat4 CurrentMatrix{glm::mat4(1.f)};
        glm::vec3 Translation{0.f};
        glm::vec3 Rotation{0.f};
        glm::vec3 Scale{1.f};

        std::string Name;

        /*~Node()
        {
            for (auto& child : Children)
            {
                delete child;
            }
        }*/

    };
}