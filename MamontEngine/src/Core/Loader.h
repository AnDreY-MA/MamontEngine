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

    BoundsData Bounds;

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

    // storage for all the data on a given glTF file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>>    meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>>         nodes;
    std::unordered_map<std::string, AllocatedImage>                images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    AllocatedBuffer materialDataBuffer;

    MEngine *creator;

    ~LoadedGLTF()
    {
        clearAll();
    };

    virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx);

private:
    void clearAll();

};

 [[nodiscard]] std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(MEngine *engine, std::string_view filePath);

 [[nodiscard]] std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath);
} // namespace MamontEngine
