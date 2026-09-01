#include "Graphics/Resources/Materials/MaterialManager.h"
#include "MaterialAllocator.h"

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

    std::shared_ptr<Material>
    MaterialManager::CreateMaterial(const EMaterialPass pass, const Material::MaterialResources &resources, const Material::MaterialConstants &data)
    {
        const auto newMaterial {std::shared_ptr<Material>(MaterialAllocator::AllocateMaterial(pass, resources, data))};

        m_Materials.push_back(newMaterial);

        return newMaterial;
    }
} // namespace MamontEngine
