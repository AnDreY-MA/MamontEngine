#pragma once

#include "pch.h"
#include <filesystem>
#include <optional>

namespace MamontEngine
{
    class MEngine;
}

struct GeoSurface
{
    uint32_t StartIndex;
    uint32_t Count;
};

struct MeshAsset
{
    std::string Name;

    std::vector<GeoSurface> Surfaces;
    GPUMeshBuffers          MeshBuffers;
};

std::optional<std::vector<std::shared_ptr<MeshAsset>>> LoadGltfMeshes(MamontEngine::MEngine *inEngine, std::filesystem::path inFilePath);
