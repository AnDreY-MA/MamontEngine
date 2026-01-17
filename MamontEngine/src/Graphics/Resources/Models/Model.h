#pragma once

#include "Graphics/Resources/Asset.h"
#include "Graphics/Resources/Materials/Material.h"
#include "Graphics/Resources/Models/Mesh.h"
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/parser.hpp>
#include "Graphics/RenderData.h"
#include "Graphics/Vulkan/Buffers/MeshBuffer.h"
#include "Utils/VkDestriptor.h"

namespace MamontEngine
{
	class MeshModel : public Asset
	{ 
    public:
        MeshModel() = default;
        explicit MeshModel(UID inID);
        explicit MeshModel(UID inID, std::string_view filePath);
        
        ~MeshModel();

        virtual void Load(std::string_view filePath) override;
        
        void Draw(DrawContext &inContext);

        void UpdateTransform(const glm::mat4 &inTransform);

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
        std::vector<Node*>                          m_Nodes;
        std::vector<std::shared_ptr<Texture>>      m_Textures;
        std::vector<std::shared_ptr<Material>> m_Materials;
        std::vector<std::shared_ptr<NewMesh>>      m_Meshes;

        MeshBuffer Buffer;

        AABB Bound;
	};
}
