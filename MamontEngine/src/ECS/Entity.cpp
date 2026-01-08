#include "ECS/Entity.h"

/*RTTR_REGISTRATION
{
    rttr::registration::class_<MamontEngine::Entity>("Entity");
}*/

namespace MamontEngine
{
    Entity::Entity(entt::entity inHandle, Scene *scene) : 
        m_EntityHandle(inHandle), m_Scene(scene)
    {
    }
}
