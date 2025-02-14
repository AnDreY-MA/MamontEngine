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
    struct RenderScene;

    std::optional<std::shared_ptr<RenderScene>> loadGltf(VkContextDevice& inDeviece, std::string_view filePath);

} // namespace MamontEngine
