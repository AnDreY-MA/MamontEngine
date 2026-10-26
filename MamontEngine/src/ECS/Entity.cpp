#include "ECS/Entity.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "Core/Log.h"
#include "Components/IDComponent.h"
#include "Components/Hierarchy.h"

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

    UID Entity::GetID()
    {
        return GetComponent<IDComponent>().ID;
    }

    void Entity::SetParent(Entity inParent)
    {
        if (!m_Scene || m_EntityHandle == entt::null)
            return;
        if (inParent && inParent == *this)
            return;

        auto &reg = m_Scene->m_Registry;

        if (auto *ownHierarchy = reg.try_get<Hierarchy>(m_EntityHandle); ownHierarchy && ownHierarchy->HasParent())
        {
            if (auto* oldParent = reg.try_get<Hierarchy>(ownHierarchy->Parent))
            {
                std::erase(oldParent->Children, m_EntityHandle);
            }
        }

        if (!inParent)
        {
            if (auto *ownHierarchy = reg.try_get<Hierarchy>(m_EntityHandle))
                ownHierarchy->Parent = entt::null;
            return;
        }

        auto &ownRel  = reg.get_or_emplace<Hierarchy>(m_EntityHandle);
        ownRel.Parent = inParent.m_EntityHandle;

        auto &parentRel = reg.get_or_emplace<Hierarchy>(inParent.m_EntityHandle);
        parentRel.Children.push_back(m_EntityHandle);
    }

    entt::entity Entity::GetParent()
    {
        if (!HasComponent<Hierarchy>())
        {
            return {};
        }

        auto &ownHierarchy = GetComponent<Hierarchy>();
        return ownHierarchy.Parent;
    }

    std::vector<Entity> Entity::GetChildrenTemp()
    {
        auto childs = GetChildren();
        std::vector<Entity> result;
        result.reserve(childs.size());

        for (auto& child : childs)
        {
            result.push_back(child);
        }

        return result;
    }

    std::span<entt::entity> Entity::GetChildren()
    {
        if (!HasComponent<Hierarchy>())
        {
            return {};     
        }

        auto &ownHierarchy = GetComponent<Hierarchy>();
        return ownHierarchy.Children;
    }
}