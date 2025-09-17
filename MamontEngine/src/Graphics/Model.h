#pragma once

#include "Vulkan/Materials/Material.h"
#include "Graphics/Mesh.h"
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include "Core/ContextDevice.h"
#include "Core/RenderData.h"

namespace MamontEngine
{
	class MeshModel
	{
    public:
        explicit MeshModel(const VkContextDevice &inDevice);
        explicit MeshModel(const VkContextDevice &inDevice, std::string_view filePath);
        
        ~MeshModel();

        void Load(std::string_view filePath);
        
        void Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext) const;

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

    private:
        void                                  LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers);
        void                                  LoadMaterials(const fastgltf::Asset &inFileAsset);
        void                                  LoadImages(const fastgltf::Asset &inFileAsset);
        void                                  LoadNodes(const fastgltf::Asset &inFileAsset);
        void                                  LoadMesh(const fastgltf::Asset &inFileAsset);

        void Clear()
        {
            m_Samplers.clear();
            m_Images.clear();
            m_Nodes.clear();
            m_Materials.clear();
            m_Meshes.clear();
        }

    private:
        const VkContextDevice &m_ContextDevice;

        std::vector<std::unique_ptr<Node>>         m_Nodes;
        std::vector<AllocatedImage>                m_Images;
        std::vector<std::shared_ptr<GLTFMaterial>> m_Materials;
        std::vector<VkSampler>                     m_Samplers;
        std::vector<std::shared_ptr<NewMesh>>      m_Meshes;

        DescriptorAllocatorGrowable DescriptorPool;
        AllocatedBuffer             MaterialDataBuffer;

        std::filesystem::path pathFile;
	};
}
