#pragma once 

#include "Graphics/Vulkan/MeshBuffer.h"
#include "Graphics/Vulkan/Materials/Material.h"

#include "Graphics/Vulkan/Image.h"

#include "Core/VkDestriptor.h"

namespace MamontEngine
{
    struct Vertex
    {
        glm::vec3 Position = {0.f, 0.f, 0.f};
        float     UV_X{0.f};
        glm::vec3 Normal = {0.f, 0.f, 0.f};
        float     UV_Y{0.f};
        glm::vec4 Color;
    };

    struct Bounds
    {
        glm::vec3 Origin  = {0.0f, 0.0f, 0.0f};
        glm::vec3 Extents = {0.0f, 0.0f, 0.0f};
        float     SpherRadius{0.0f};
    };

    struct VkContextDevice;

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

        MeshBuffer Buffer;

    };

    struct Node
    {
        Node *Parent{nullptr};
        std::vector<Node *> Children;
        NewMesh            *Mesh;

        glm::mat4 Matrix;
        /*glm::vec3 Translation{};
        glm::vec3 Scale{1.f};*/

        std::string Name;

        ~Node()
        {
            for (auto& child : Children)
            {
                delete child;
            }
        }

    };
}