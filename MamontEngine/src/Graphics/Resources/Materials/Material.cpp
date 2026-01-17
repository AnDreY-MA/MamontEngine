#include "Graphics/Resources/Materials/Material.h"

namespace MamontEngine
{
    Material::~Material()
    {
        MaterialSet = VK_NULL_HANDLE;
    }
    Material::Material(Material&& other)
    {
        Swap(other);
    }

    Material &Material::operator=(Material &&other)
    {
        Swap(other);
        return *this;
    }

    void Material::Swap(Material& other)
    {
        std::swap(MaterialSet, other.MaterialSet);
        std::swap(PassType, other.PassType);
        std::swap(Resources, other.Resources);
        std::swap(Constants, other.Constants);
        std::swap(Index, other.Index);
        std::swap(BufferOffset, other.BufferOffset);
        std::swap(Name, other.Name);
    }
}