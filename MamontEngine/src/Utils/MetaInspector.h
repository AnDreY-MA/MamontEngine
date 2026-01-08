#pragma once

#include <entt/meta/meta.hpp>
#include <imgui/imgui.h>
#include "Math/Transform.h"
#include "UI/UI.h"

using entt::literals::operator""_hs;

constexpr entt::hashed_string f_Inspect = "Inspect"_hs;

namespace MetaInspectors
{
    using namespace MamontEngine;
    template <typename Type>
    inline void MetaInspect(const std::string &name, Type &value, const entt::meta_data &meta)
    {
        ImGui::TextColored(ImColor(255, 0, 0), "Missing MetaInspect for: '%s'", meta.type().info().name().data());
    }


    template <typename Type>
    static void Inspect(const char *name, entt::meta_any &value, const entt::meta_data &meta)
    {
        Type &typedValue = value.cast<Type &>();
        MetaInspect<Type>(name, typedValue, meta);
    }

    template<>
    inline void MetaInspect<Transform>(const std::string &name, Transform &value, const entt::meta_data &meta)
    {
        ImGui::Text(name.c_str());

        MUI::DrawVec3Control("Position", value.Position);
        MUI::DrawVec3Control("Rotation", value.Rotation);
        MUI::DrawVec3Control("Scale", value.Scale, 1.f);
    }
}

template <class Type>
static auto TypeInspect()
{
    auto type = entt::meta_factory<Type>();
    type.template func<&MetaInspectors::Inspect<Type>>(f_Inspect); // Inspect string
    return type;
}
