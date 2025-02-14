#include "ECS/Entity.h"

namespace MamontEngine
{
    Entity::Entity(entt::entity inHandle, Scene *scene) : 
        m_EntityHandle(inHandle), m_Scene(scene)
    {
    }
}
