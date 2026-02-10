#pragma once

#include <entt/meta/meta.hpp>
#include <imgui/imgui.h>
#include "Math/Transform.h"
#include "Math/Color.h"
#include "UI/UI.h"
#include "Physics/Body/Rigidbody.h"

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

    template <>
    inline void MetaInspect<float>(const std::string &name, float &value, const entt::meta_data &meta)
    {
        ImGui::SameLine();
        ImGui::DragFloat(name.c_str(), &value, 0.2f);
    }

    template <typename Type>
    static void InspectEnum(const char* name, entt::meta_any& value, const entt::meta_data& meta)
    {
        //Type type = value.cast<T>();
    }

    template<>
    inline void MetaInspect<Transform>(const std::string &name, Transform &value, const entt::meta_data &meta)
    {
        //ImGui::Text(name.c_str());

        MUI::DrawVec3Control("Position", value.Position);
        MUI::DrawQuatControl("Rotation", value.Rotation);
        MUI::DrawVec3Control("Scale", value.Scale, 1.f);
    }
    template <>
    inline void MetaInspect<Color>(const std::string &name, Color &value, const entt::meta_data &meta)
    {
        ImGui::ColorEdit4(name.c_str(), value.Data());
    }

    template <>
    inline void MetaInspect<glm::vec3>(const std::string &name, glm::vec3 &value, const entt::meta_data &meta)
    {
        MUI::DrawVec3Control(name.c_str(), value);
    }

    template <>
    inline void MetaInspect<HeroPhysics::Rigidbody>(const std::string &name, HeroPhysics::Rigidbody &value, const entt::meta_data &meta)
    {
        float mass = value.GetMass();
        if (ImGui::DragFloat("Mass", &mass, 0.2f))
        {
            value.SetMass(mass);
        }
        
    }
}

template <class Type>
static auto TypeInspect()
{
    auto type = entt::meta_factory<Type>();
    type.template func<&MetaInspectors::Inspect<Type>>(f_Inspect);
    return type;
}
