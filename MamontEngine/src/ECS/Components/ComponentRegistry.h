#pragma once

#include <entt/entt.hpp>

namespace MamontEngine
{
	class ComponentRegistry
	{
    public:
		using EmplaceFn = void(*)(entt::registry&, entt::entity);
        using HasFn     = bool(*)(entt::registry&, entt::entity);

		struct Entry
		{
            std::string_view Name;
            entt::id_type    Id;
            EmplaceFn        Emplace;
            HasFn            Has;
		};

		static ComponentRegistry &Instance();

		template<typename T>
		void Register(std::string_view inName)
		{
            const entt::id_type id = entt::type_id<T>().hash();
			for (const auto& e : m_Entries)
			{
                if (e.Id == id) return;
			}

			m_Entries.push_back({inName,
                                      id,
                                      [](entt::registry &r, entt::entity e) {
                                          if (!r.all_of<T>(e))
                                              r.emplace<T>(e);
                                      },
                                      [](entt::registry &r, entt::entity e) { return r.all_of<T>(e); }
				}
			);
		}

		std::span<const Entry> GetEntries() const { return m_Entries; }

	private:
        std::vector<Entry> m_Entries;
	};
}