#pragma once

#include <entt/entt.hpp>
#include "Core/UID.h"

namespace MamontEngine
{
    class Entity;

	class Scene
	{
    public:

		Entity CreateEntity(std::string_view inName = std::string_view());
        void   DestroyEntity(Entity inEntity);

	private:

		Entity CreateEntity(UID inId, std::string_view inName);

	private:
        entt::registry m_Registry;

		std::unordered_map<UID, entt::entity> m_Entities;

		friend class Entity;
	};

} // namespace MamontEngine
