#pragma once

#include "Graphics/Resources/Material.h"

namespace MamontEngine
{
    namespace MaterialAllocator
    {
        void      Init();
        void      Destroy();

        Material *AllocateMaterial(const EMaterialPass pass, const Material::MaterialResources &resources, const Material::MaterialConstants &data);

        void Free(Material *material);

        void Update(const void* inData, const size_t inOffset);

    }
} // namespace MamontEngine
