#pragma once

#include "pch.h"
#include <filesystem>
#include <optional>
#include "RenderData.h"
#include "Graphics/Mesh.h"

namespace MamontEngine
{
    class MEngine;



struct LoadedGLTF : public IRenderable
{

    std::unordered_map<std::string, std::shared_ptr<Mesh>>    meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>>         nodes;
    std::unordered_map<std::string, AllocatedImage>                images;
    std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> materials;

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

std::optional<std::shared_ptr<LoadedGLTF>> loadGltf(MEngine *engine, std::string_view filePath);

} // namespace MamontEngine
