#pragma once

#include "Graphics/Resources/Material.h"
#include "Graphics/Resources/Models/Mesh.h"
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
        
        void Draw(DrawContext &inContext);

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

        Material* GetMaterial(size_t index) const
        {
            return m_Materials.at(index).get();
        }

        UID ID;

    private:
        void                                  LoadMaterials(const fastgltf::Asset &inFileAsset, const std::vector<VkSampler> &inSamplers);
        void                                  LoadImages(const fastgltf::Asset &inFileAsset, const std::vector<VkSampler>& inSamplers);
        void                                  LoadNodes(const fastgltf::Asset &inFileAsset);
        void                                  LoadMesh(const fastgltf::Asset &inFileAsset);

        std::vector<VkSampler> LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers);

        void Clear();

    private:
        const VkContextDevice &m_ContextDevice;

        std::vector<std::shared_ptr<Node>>         m_Nodes;
        std::vector<Texture>               m_Textures;
        //std::vector<Texture>                       m_Textures;
        std::vector<std::shared_ptr<Material>> m_Materials;
        std::vector<std::shared_ptr<NewMesh>>      m_Meshes;

        DescriptorAllocatorGrowable DescriptorPool;
        AllocatedBuffer             MaterialDataBuffer;

        MeshBuffer Buffer;

        std::filesystem::path pathFile;

	};
}
