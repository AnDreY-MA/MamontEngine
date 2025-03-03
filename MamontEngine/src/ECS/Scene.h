#pragma once

#include <entt/entt.hpp>
#include "Core/UID.h"
#include "Core/Camera.h"
#include "ECS/SceneRenderer.h"

namespace MamontEngine
{
    class Entity;
    struct VkContextDevice;

	class Scene
	{
    public:
        Scene() = default;
        explicit Scene(const std::shared_ptr<SceneRenderer> &inSceneRenderer);
        ~Scene();

        void Init(VkContextDevice& inContextDevice);

		Entity CreateEntity(std::string_view inName = std::string_view());

		void Update();
		
        void   DestroyEntity(Entity inEntity);

		entt::registry& GetRegistry()
		{
            return m_Registry;
		}

	private:
		Entity CreateEntity(UID inId, std::string_view inName);

	private:
        entt::registry m_Registry;

		std::shared_ptr<SceneRenderer> m_SceneRenderer;

		std::unordered_map<UID, entt::entity> m_Entities;

		friend class Entity;
        friend class SceneHierarchyPanel;
	};

} // namespace MamontEngine
