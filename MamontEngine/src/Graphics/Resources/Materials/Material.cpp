#include "Graphics/Resources/Materials/Material.h"

namespace MamontEngine
{
    Material::~Material()
    {
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
        std::swap(PassType, other.PassType);
        std::swap(Constants, other.Constants);
        std::swap(Index, other.Index);
        std::swap(Name, other.Name);
    }
}