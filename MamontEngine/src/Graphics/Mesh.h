#pragma once 

#include "Graphics/Vulkan/GPUBuffer.h"
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

    struct GeoSurface
    {
        uint32_t StartIndex{0};
        uint32_t Count{0};

        Bounds Bound;

        std::shared_ptr<GLTFMaterial> Material;
    };

    struct VkContextDevice;

	struct Mesh
    {
    public:
        explicit Mesh(VkContextDevice &inDevice);

        ~Mesh();

        struct Primitive
        {
            std::vector<GeoSurface> Surfaces;
            MeshBuffer          Buffers;
        };

        struct Node
        {
            std::weak_ptr<Node>                 Parent;
            std::vector<std::shared_ptr<Node>>  Children;
            glm::mat4                           LocalTransform;
            glm::mat4                           WorldTransform{1.f};

            std::shared_ptr<Primitive> Primitive;

            inline void RefreshTransform(const glm::mat4& inParentMatrix)
            {
                WorldTransform = inParentMatrix * LocalTransform;
            }
        };

        std::vector<std::shared_ptr<Node>> Nodes;
        std::unordered_map<std::string, std::shared_ptr<Primitive>>    Primitives; 
        std::unordered_map<std::string, AllocatedImage>                Images;
        std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> Materials;
        std::vector<VkSampler>                                         Samplers;

        DescriptorAllocatorGrowable DescriptorPool;

        AllocatedBuffer MaterialDataBuffer; 

        void Draw(const glm::mat4 &inTopMatrix, struct DrawContext &inContext);

    private:
        void ClearAll();
        VkContextDevice &Device;
        //std::vector<Vertex> Vertices;
    };
}