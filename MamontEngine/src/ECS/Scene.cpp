#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "Utils/Serialization.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Resources/Models/Model.h"
#include "Core/ContextDevice.h"
#include "Core/Log.h"
#include <execution>
#include <entt/core/hashed_string.hpp>
#include "entt/meta/meta.hpp"
#include "Utils/Reflection.h"
#include <fstream>
#include <ostream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/polymorphic.hpp>

/*META_INIT(glm);

IMPLEMENT_META_INIT(glm)
{
    META_TYPE(glm::vec3);
}
FINISH_REFLECT()*/


namespace MamontEngine
{
    Scene::Scene() 
    {
        using hs = entt::hashed_string;
        entt::meta_factory<MeshModel>{}
        .type(hs{"Model"}, "Model").base<Asset>();
        entt::meta_factory<Transform>
        {}.type(hs{"Transform"}, "Transform");


        entt::meta_factory<TransformComponent>{}
                .type(hs{"TransformComponent "}, "Transform Component ")
                .data<&TransformComponent::Transform>(hs{"Transform"}, "Transform");

       entt::meta_factory<MeshComponent>{}
           .type(hs{"MeshComponent"}, "Mesh component")
           .data<&MeshComponent::Mesh>(hs{"Model"}, "Model");

        entt::meta_factory<DirectionLightComponent>{}
               .type(hs{"DirectionLightComponent"}, "Direction Light Component")
               .base<LightComponent>()
               .data<&DirectionLightComponent::Color>(hs{"Color"}, "Color")
               .data<&DirectionLightComponent::Intensity>(hs{"Intensity"}, "Intensity");
    }

    Scene::~Scene()
    {
        Clear();
    }

    void Scene::Init()
    {
        Load();
    }

    void Scene::Clear()
    {

        m_Registry.clear();
    }

    void Scene::Save()
    {
        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.sasset";
        const bool result = Save(fileScene);

        Log::Info("Saved scene is {}", result ? "Success" : "Failed");
    }

    bool Scene::Save(std::string_view inFile)
    {
        std::ofstream file(inFile.data(), std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("Cannot open file: {}", inFile.data());
            return false;
        }

        cereal::BinaryOutputArchive serializer(file);

        entt::snapshot{m_Registry}.get<entt::entity>(serializer).get<IDComponent>(serializer).get<TagComponent>(serializer).get<TransformComponent>(serializer)
            .get<MeshComponent>(serializer).get<DirectionLightComponent>(serializer);

        file.close();

        return true;
    }

    void Scene::Load()
    {
        Clear();
        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.sasset";
        const bool result = Load(fileScene);

        Log::Info("Load Scene is {}", result ? "Success" : "Failed");
    }

    bool Scene::Load(std::string_view inFile)
    {
        std::ifstream file(inFile.data(), std::ios::binary);
        if (!file.is_open())
        {
            Log::Error("Cannot open file: {}", inFile.data());
            return false;
        }

        cereal::BinaryInputArchive serializer(file);

        entt::snapshot_loader{m_Registry}
                .get<entt::entity>(serializer)
                .get<IDComponent>(serializer)
                .get<TagComponent>(serializer)
                .get<TransformComponent>(serializer)
                .get<MeshComponent>(serializer)
                .get<DirectionLightComponent>(serializer);

        file.close();

        return true;
    }

    void Scene::Update()
    {
        const auto meshes = m_Registry.view<MeshComponent, TransformComponent>();
        for (const auto &&[entity, meshComponent, transform] : meshes.each())
        {
            if (meshComponent.Mesh)
            {
                meshComponent.Mesh->UpdateTransform(
                        transform.Matrix());
                //transform.IsDirty = false;
            }
        }
    }

    void Scene::DestroyEntity(Entity inEntity)
    {
        if (!m_Registry.valid(inEntity))
        {
            fmt::println("Entity is not valid");
            return;
        }

        if (!m_Registry.storage<entt::entity>().contains(inEntity))
        {
            fmt::println("Entity {} not in registry storage", inEntity.GetComponent<TagComponent>().Tag);
            return;
        }

        RemoveComponent<MeshComponent>(inEntity);

        m_Registry.destroy(inEntity);

    }

    Entity Scene::CreateEntity(std::string_view inName)
    {
        return CreateEntity(UID(), inName);
    }

    Entity Scene::CreateEntity(UID inId, std::string_view inName)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<IDComponent>().ID = inId;
        entity.AddComponent<TagComponent>(inName.empty() ? "Enity" : inName);
        entity.AddComponent<TransformComponent>();

        Log::Info("Created entity {}", inName);

        return entity;
    }
    
    Entity Scene::GetEntity(UID Id)
    {
        const auto &view = m_Registry.view<IDComponent>();
        view.each(
                [&](entt::entity entity, const IDComponent &id)
                {
                    if (id.ID == Id)
                    {
                        return Entity{entity, this};
                    }
                });

        Log::Warn("Entity not found");
        return {};

    }

    template<>
    void Scene::RemoveComponent<MeshComponent>(Entity inEntity)
    {
        m_Registry.remove<MeshComponent>(inEntity);
    }

} // namespace MamontEngine