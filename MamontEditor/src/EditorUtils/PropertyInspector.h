#pragma once

#include <entt/entt.hpp>

namespace MamontEditor
{
    void InspectProperty(entt::meta_any &property, entt::meta_data &propertyMeta, const entt::id_type propertyID);

    void InspectType(const std::string &name, entt::meta_any &value, entt::meta_data &meta);

    void InspectComponent(entt::entity owner, entt::meta_any &data);
}