#include "ECS/Components/ComponentRegistry.h"

namespace MamontEngine
{
    ComponentRegistry& ComponentRegistry::Instance()
    {
        static ComponentRegistry instance;
        return instance;
    }
}