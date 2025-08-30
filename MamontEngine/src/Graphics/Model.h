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
        MeshModel(const VkContextDevice &inDevice) : m_ContextDevice(inDevice)
        {
        }
        ~MeshModel();

        void Load(std::string_view filePath);
        
        void Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext);

        void UpdateTransform(const glm::mat4 &inTransform);

        const size_t GetSizeMaterials() const
        {
            return Materials.size();
        }
        const size_t GetSizeMeshes() const
        {
            return m_Meshes.size();
        }
        const size_t GetSizeNodes() const
        {
            return m_Nodes.size();
        }

    private:
        void                                  LoadSamplers(VkDevice inDevice, const std::vector<fastgltf::Sampler> &samplers);
        void                                  LoadMaterials(fastgltf::Asset &inFileAsset);
        void                                  LoadImages(fastgltf::Asset &inFileAsset);
        void                                  LoadNodes(fastgltf::Asset &inFileAsset);
        std::vector<std::shared_ptr<NewMesh>> LoadMesh(fastgltf::Asset &inFileAsset);

    private:
        const VkContextDevice &m_ContextDevice;

        std::vector<std::unique_ptr<Node>>         m_Nodes;
        std::vector<AllocatedImage>                m_Images;
        std::vector<std::shared_ptr<GLTFMaterial>> Materials;
        std::vector<VkSampler>                     Samplers;
        std::vector<std::shared_ptr<NewMesh>>      m_Meshes;

        DescriptorAllocatorGrowable DescriptorPool;
        AllocatedBuffer             MaterialDataBuffer;
	};
}