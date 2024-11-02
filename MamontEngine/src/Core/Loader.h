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

std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath);
} // namespace MamontEngine
