#pragma once

#include "pch.h"
#include <filesystem>
#include <optional>
#include "Core/RenderData.h"
#include "Graphics/Mesh.h"

namespace MamontEngine
{
    class MEngine;
    struct VkContextDevice;

    std::optional<std::shared_ptr<Mesh>> loadGltf(std::string_view filePath);

    std::optional<std::shared_ptr<Mesh>> loadGltf(VkContextDevice &inDeviece, std::string_view filePath);

} // namespace MamontEngine
