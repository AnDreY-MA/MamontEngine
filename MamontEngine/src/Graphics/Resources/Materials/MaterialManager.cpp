#include "Graphics/Resources/Materials/MaterialManager.h"
#include "Graphics/Resources/Texture.h"
#include "MaterialAllocator.h"
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <fstream>
#include "Core/Log.h"

namespace MamontEngine
{
    MaterialManager* MaterialManager::s_Instance = nullptr;

    MaterialManager::MaterialManager()
    {
        MaterialAllocator::Init();
        m_Materials.reserve(2 * 1024);

        s_Instance = this;
    }
    
    MaterialManager::~MaterialManager()
    {
        MaterialAllocator::Destroy();
        s_Instance = nullptr;
    }

    void MaterialManager::SetupDefaults()
    {
        const std::shared_ptr<Material>& newMaterial{std::shared_ptr<Material>(CreateWhiteMaterial())};
        m_Materials.push_back(newMaterial);
        const std::string           filePath = DEFAULT_ASSETS_DIRECTORY + "Materials/M_White.masset";
        std::ofstream     file(filePath.data(), std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("Cannot open file: {}", filePath.data());
            return;
        }

        cereal::BinaryOutputArchive archive(file);

        archive(newMaterial);
    }

    std::shared_ptr<Material>
    MaterialManager::CreateMaterial(const EMaterialPass pass, const Material::MaterialResources &resources, const Material::MaterialConstants &data)
    {
        const auto newMaterial {std::shared_ptr<Material>(MaterialAllocator::AllocateMaterial(pass, resources, data))};

        m_Materials.push_back(newMaterial);

        return newMaterial;
    }

    Material *CreateWhiteMaterial()
    {
        const EMaterialPass pass{EMaterialPass::MAIN_COLOR};

        const auto whiteTexture = CreateWhiteTexture();

        Material::MaterialResources resources;
        resources.ColorTexture = whiteTexture;
        resources.MetalRoughTexture = whiteTexture;
        resources.NormalTexture = whiteTexture;
        resources.OcclusionTexture = whiteTexture;
        resources.EmissiveTexture = whiteTexture;
        const Material::MaterialConstants matParameters{.ColorFactors = glm::vec4(1.f)};

        Material *newMaterial = MaterialAllocator::AllocateMaterial(pass, resources, matParameters);
        newMaterial->Name     = "M_White";
        return newMaterial;
    }
} // namespace MamontEngine
