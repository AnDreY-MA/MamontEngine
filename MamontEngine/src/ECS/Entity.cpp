#include "ECS/Entity.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "Core/Log.h"

namespace MamontEngine
{
    Entity::Entity(entt::entity inHandle) 
        : m_EntityHandle(inHandle)
    {
    }

    Entity::Entity(entt::entity inHandle, Scene *scene) : 
        m_EntityHandle(inHandle), m_Scene(scene)
    {
    }
}