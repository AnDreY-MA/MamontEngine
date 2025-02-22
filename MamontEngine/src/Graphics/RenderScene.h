#pragma once

#include "pch.h"
#include <filesystem>
#include <optional>
#include "Core/RenderData.h"
#include "Graphics/Mesh.h"
#include "Core/ContextDevice.h"

namespace MamontEngine
{

	struct RenderScene
    {
        explicit RenderScene(VkContextDevice &inDevice);
        ~RenderScene();

        std::unordered_map<std::string, std::shared_ptr<MeshTest>>         Meshes;
        std::unordered_map<std::string, std::shared_ptr<Node>>         Nodes;
        std::unordered_map<std::string, AllocatedImage>                Images;
        std::unordered_map<std::string, std::shared_ptr<GLTFMaterial>> Materials;

        std::vector<std::shared_ptr<Node>> TopNodes;

        std::vector<VkSampler> Samplers;

        DescriptorAllocatorGrowable DescriptorPool;

        AllocatedBuffer MaterialDataBuffer;

        void Draw(const glm::mat4 &topMatrix, DrawContext &ctx);

    private:
        void ClearAll();

        VkContextDevice &Device;
	};
}