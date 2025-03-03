#pragma once

#include "ECS/Scene.h"

#include "entt/entt.hpp"
#include "Components/IDComponent.h"

namespace MamontEngine
{
	class Entity
	{
    public:
        Entity() = default;
        Entity(const Entity &other) = default;
        Entity(entt::entity inHandle, Scene *scene);

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            T &component = m_Scene->m_Registry.emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
            return component;
        }

        template <typename T>
        T& GetComponent()
        {
            return m_Scene->m_Registry.get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent()
        {
            return m_Scene->m_Registry.any_of<T>(m_EntityHandle);
        }

        template<typename T>
        void RemoveComponent()
        {
            m_Scene->m_Registry.remove<T>(m_EntityHandle);
        }

        UID GetID()
        {
            return GetComponent<IDComponent>().ID;
        }

        operator bool() const
        {
            return m_EntityHandle != entt::null;
        }

        operator entt::entity() const
        {
            return m_EntityHandle;
        }

        bool operator==(const Entity& other) const
        {
            return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
        }

        bool operator!=(const Entity& other) const
        {
            return !(*this == other);
        }

	private:
        entt::entity m_EntityHandle{entt::null};
        Scene *m_Scene{nullptr};
	};
}