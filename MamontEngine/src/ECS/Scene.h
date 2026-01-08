#pragma once

#include <entt/entt.hpp>
#include "Core/UID.h"
#include "Core/Camera.h"
#include "Utils/Serializer.h"
#include "ECS/Components/MeshComponent.h"

namespace MamontEngine
{
    class Entity;
    struct VkContextDevice;

	class Scene
	{
    public:
        //Scene() = default;
        explicit Scene();

        ~Scene();

        void Init();

		void Save();
        void Load();

		Entity CreateEntity(std::string_view inName = std::string_view());

		void Update();

		template<typename T>
        void RemoveComponent(Entity inEntity)
		{
            m_Registry.remove<T>(inEntity);
		}
		template<>
		void RemoveComponent<MeshComponent>(Entity inEntity);
		
        void   DestroyEntity(Entity inEntity);

		entt::registry& GetRegistry()
		{
            return m_Registry;
		}

		const entt::registry &GetRegistry() const
        {
            return m_Registry;
        }
		Entity GetEntity(UID Id);
        const Entity& GetEntity(UID Id) const;

	private:
		Entity CreateEntity(UID inId, std::string_view inName);

		void Clear();

	private:
        entt::registry m_Registry;

		std::unordered_map<UID, Entity> m_Entities;

		friend class Entity;
        friend class SceneHierarchyPanel;
        friend class Serializer;
	};

} // namespace MamontEngine
