#include "Graphics/Resources/MaterialAllocator.h"
#include "Graphics/Resources/Material.h"

namespace MamontEngine
{
    namespace MaterialAllocator
    {
        Material *AllocateMaterial()
        {
            Material *newMaterial = new Material();

            return newMaterial;
        }
    } // namespace MaterialAllocator
} // namespace MamontEngine
