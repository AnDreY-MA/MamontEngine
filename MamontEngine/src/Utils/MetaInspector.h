#pragma once

#include <entt/meta/meta.hpp>
#include <imgui/imgui.h>
#include "Math/Transform.h"
#include "Math/Color.h"
#include "UI/UI.h"
#include "Physics/Body/Rigidbody.h"

using entt::literals::operator""_hs;

constexpr entt::hashed_string f_Inspect = "Inspect"_hs;
constexpr entt::hashed_string p_DisplayName = "DisplayName"_hs;

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
    inline void MetaInspect<bool>(const std::string &name, bool &value, const entt::meta_data &meta)
    {
        ImGui::Text(name.c_str());
        ImGui::SameLine();
        ImGui::Checkbox(("##" + name).c_str(), &value);
    }
    template <>
    inline void MetaInspect<float>(const std::string &name, float &value, const entt::meta_data &meta)
    {
        ImGui::Text(name.c_str());
        ImGui::SameLine();
        ImGui::DragFloat(("##" + name).c_str(), &value, 0.2f);
    }


    template <typename Type>
    static void InspectEnum(const char* name, entt::meta_any& value, const entt::meta_data& meta)
    {
        Type type = value.cast<Type>();

        struct TypeID
        {
            unsigned Id;
            std::string Name;
        };

        static std::vector<TypeID> enumNames = std::vector<TypeID>();
        enumNames.resize(10);
        int                        amount{0};
        std::string                active{"None"};

        for (auto&& [enumID, elementType] : value.type().data())
        {
            auto instance = elementType.get(value);

            std::string name = "UNKNOWN";
            name = elementType.name();
            enumNames[amount] = {enumID, name};
            amount++;

            if (elementType.get({}).cast<Type>() == type)
            {
                active = name;
            }
        }
        ImGui::Text(name);
        ImGui::SameLine();

        if (!ImGui::BeginCombo(("##" + std::string(name)).c_str(), active.c_str()))
            return;

        for (int i = 0; i < amount; ++i)
        {
            if (ImGui::Selectable(enumNames[i].Name.c_str(), enumNames[i].Name == active))
            {
                value = value.type().data(enumNames[i].Id).get({});
                
                ImGui::EndCombo();
                return;
            }
        }

        ImGui::EndCombo();

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
        ImGui::Text(name.c_str());
        ImGui::SameLine();
        ImGui::ColorEdit4(("##" + name).c_str(), value.Data());
    }

    template <>
    inline void MetaInspect<glm::vec3>(const std::string &name, glm::vec3 &value, const entt::meta_data &meta)
    {
        MUI::DrawVec3Control(name.c_str(), value);
    }

    template <>
    inline void MetaInspect<HeroPhysics::Rigidbody>(const std::string &name, HeroPhysics::Rigidbody &value, const entt::meta_data &meta)
    {
        /*const char* motionTypeName = "MotionType";

        if (ImGui::BeginCombo(motionTypeName, "r"))
        {

        }*/
    }
}

template <class Type>
static auto TypeInspect()
{
    auto type = entt::meta_factory<Type>();
    type.template func<&MetaInspectors::Inspect<Type>>(f_Inspect);
    return type;
}
