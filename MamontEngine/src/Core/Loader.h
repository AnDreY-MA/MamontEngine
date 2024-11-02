#pragma once

#include "pch.h"
#include <filesystem>
#include <optional>
#include "RenderData.h"

namespace MamontEngine
{
    class MEngine;

    struct GLTFMaterial
    {
        MaterialInstance Data;
    };

    struct GeoSurface
    {
        uint32_t StartIndex;
        uint32_t Count;

        std::shared_ptr<GLTFMaterial> Material;
    };

    struct MeshAsset
    {
        std::string             Name;
        std::vector<GeoSurface> Surfaces;
        GPUMeshBuffers          MeshBuffers;
    };

    struct LoadedGLTF : public IRenderable
    {
    public:
        ~LoadedGLTF()
        {
            ClearAll();
        }

        virtual void Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext);


        std::unordered_map<std::string, std::shared_ptr<MeshAsset>> Meshes;
        std::unordered_map<std::string, std::shared_ptr<Node>>      Nodes;
        std::unordered_map<std::string, AllocatedImage>             Images;
        std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> Materials;

        std::vector<std::shared_ptr<Node>> TopNodes;

        std::vector<VkSampler> Samplers;

        VkDescriptor::DescriptorAllocatorGrowable DescriptorPool;
        AllocatedBuffer                           MaterialDataBuffer;

        MEngine *Creator;

    private:
        void ClearAll()
        {
        }
    };

    std::optional<std::shared_ptr<LoadedGLTF>> LoadGltf(MEngine *inEngine, std::string_view inFilePath);
} // namespace MamontEngine
