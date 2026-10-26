#pragma once

#include "entt/entt.hpp"

namespace MamontEngine
{
	struct Hierarchy
	{
        entt::entity Parent{entt::null};
        std::vector<entt::entity> Children;

		inline bool HasParent() const { return Parent != entt::null; }
	};
}