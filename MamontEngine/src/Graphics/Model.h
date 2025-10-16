#pragma once

#include "Vulkan/Materials/Material.h"
#include "Graphics/Mesh.h"
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include "Core/ContextDevice.h"
#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Buffers/MeshBuffer.h"

namespace MamontEngine
{
	class MeshModel
	{
    public:
        explicit MeshModel(const VkContextDevice &inDevice, UID inID);
        explicit MeshModel(const VkContextDevice &inDevice, UID inID, std::string_view filePath);
        
        ~MeshModel();

        void Load(std::string_view filePath);
        
        void Draw(DrawContext &inContext) const;

        void UpdateTransform(const glm::mat4 &inTransform, const glm::vec3 &inLocation, const glm::vec3 &inRotation, const glm::vec3 &inScale);

        const size_t GetSizeMaterials() const
        {
            return m_Materials.size();
        }
        const size_t GetSizeMeshes() const
        {
            return m_Meshes.size();
        }
        const size_t GetSizeNodes() const
        {
            return m_Nodes.size();
        }

        const std::string GetPathFile() const
        {
            return pathFile.string();
        }

        UID ID;

    private:
        void                                  LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers);
        void                                  LoadMaterials(const fastgltf::Asset &inFileAsset);
        void                                  LoadImages(const fastgltf::Asset &inFileAsset);
        void                                  LoadNodes(const fastgltf::Asset &inFileAsset);
        void                                  LoadMesh(const fastgltf::Asset &inFileAsset);

        void Clear();

    private:
        const VkContextDevice &m_ContextDevice;

        std::vector<std::shared_ptr<Node>>         m_Nodes;
        std::vector<AllocatedImage>                m_Images;
        std::vector<std::shared_ptr<GLTFMaterial>> m_Materials;
        std::vector<VkSampler>                     m_Samplers;
        std::vector<std::shared_ptr<NewMesh>>      m_Meshes;

        DescriptorAllocatorGrowable DescriptorPool;
        AllocatedBuffer             MaterialDataBuffer;

        MeshBuffer Buffer;

        std::filesystem::path pathFile;

	};
}
