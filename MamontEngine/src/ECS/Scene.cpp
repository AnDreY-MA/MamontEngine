#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Resources/Models/Model.h"
#include "Core/ContextDevice.h"
#include "Core/Log.h"
#include <execution>
#include <entt/core/hashed_string.hpp>
#include "entt/meta/meta.hpp"
#include "Utils/Reflection.h"

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
        for (auto& [id, entity] : m_Entities)
        {
            RemoveComponent<MeshComponent>(entity);
        }
        m_Registry.clear();
        m_Entities.clear();
    }

    void Scene::Save()
    {
        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.json";
        Serializer::SaveToFile(m_Registry, fileScene);
    }

    void Scene::Load()
    {
        Clear();
        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.json";
        Serializer::LoadFromFile(this, fileScene);
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

        m_Entities.erase(inEntity.GetID());
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

        m_Entities.emplace(inId, entity);

        Log::Info("Created entity {}", inName);

        return entity;
    }
    
    Entity Scene::GetEntity(UID Id)
    {
        if (auto it = m_Entities.find(Id); it != m_Entities.end())
            return it->second;
        return {};
    }
    const Entity& Scene::GetEntity(UID Id) const
    {
        if (auto it = m_Entities.find(Id); it != m_Entities.end())
            return it->second;
        return {};
    }

    template<>
    void Scene::RemoveComponent<MeshComponent>(Entity inEntity)
    {
        m_Registry.remove<MeshComponent>(inEntity);
    }

} // namespace MamontEngine