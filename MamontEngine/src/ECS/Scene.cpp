#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "Core/Engine.h"
#include "Core/ContextDevice.h"
#include "Core/Log.h"


#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Resources/Models/Model.h"
#include "Asset/AssetManager.h"

#include <execution>
#include <entt/core/hashed_string.hpp>
#include "entt/meta/meta.hpp"
#include <fstream>
#include <ostream>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/types/polymorphic.hpp>

#include "Physics/Collision/BoxCollision.h"
#include "Physics/Collision/SphereCollision.h"
#include "Physics/Body/Rigidbody.h"
#include "Physics/PhysicsSystem.h"

#include "ECS/Components/IDComponent.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/Hierarchy.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "ECS/Components/DirectionLightComponent.h"
#include "ECS/Components/PointLightComponent.h"
#include "ECS/Components/CameraComponent.h"
#include "ECS/Components/RigidbodyComponent.h"
#include "ECS/Components/ScriptComponent.h"
#include "ECS/Components/AudioComponent.h"
#include "ECS/Components/AudioListenerComponent.h"

#include "Audio/Sound.h"

#include "Utils/Serialization.h"
#include "Utils/Reflection.h"

META_INIT(void) 
IMPLEMENT_META_INIT(void)
{
    META_TYPE(float);
    META_TYPE(bool);
    META_TYPE(std::string);
    META_TYPE(std::filesystem::path);
    META_TYPE(entt::entity);
    META_TYPE(MamontEngine::Transform);
    META_TYPE(MamontEngine::Color);
}
FINISH_REFLECT()

META_INIT(glm);

IMPLEMENT_META_INIT(glm)
{
    META_TYPE(glm::vec3);
}
FINISH_REFLECT()


#define ALL_COMPONENTS(serializer) get<IDComponent>(serializer).get<TagComponent>(serializer).get<TransformComponent>(serializer) \
        .get<MeshComponent>(serializer)                                                                                                                    \
            .get<DirectionLightComponent>(serializer)                                                                                                          \
            .get<PointLightComponent>(serializer)                                                                                                                        \
            .get<RigidbodyComponent>(serializer)                                                                                                               \
            .get<HeroPhysics::BoxCollision>(serializer)                                                                                                        \
            .get<HeroPhysics::SphereCollision>(serializer) \
            .get<ScriptComponent>(serializer) \
            .get<CameraComponent>(serializer) \
            .get<AudioListenerComponent>(serializer)


namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(AudioListenerComponent, "Audio Listener Component")
    {
        meta.base<Component>();
        meta.data<&AudioListenerComponent::IsEnabled, entt::as_ref_t>("Enable");
    }
    FINISH_REFLECT()

    IMPLEMENT_REFLECT_COMPONENT(MeshComponent, "Mesh Component")
    {
        meta.data<&MeshComponent::Mesh, entt::as_ref_t>("Model"_hs);
    }
    FINISH_REFLECT()

    IMPLEMENT_REFLECT_COMPONENT(DirectionLightComponent, "DirectionLightComponent")
    {
        meta.base<LightComponent>();
        meta.data<&DirectionLightComponent::Color>("Color");
        meta.data<&DirectionLightComponent::Intensity, entt::as_ref_t>("Intensity");
    }
    FINISH_REFLECT()


    static void UpdateTransform(entt::entity entity, entt::registry& registry, TransformComponent& transformComponent)
    {
        auto hierarchy = registry.try_get<Hierarchy>(entity);
        if (hierarchy)
        {
            if (hierarchy->HasParent())
            {
                auto parentTransform = registry.try_get<TransformComponent>(hierarchy->Parent);
                if (parentTransform)
                {
                    transformComponent.SetWorldMatrix(parentTransform->Matrix());
                }
                else
                {
                    transformComponent.SetWorldMatrix(glm::mat4(1.f));
                }
            }
            else
            {
                transformComponent.SetWorldMatrix(glm::mat4(1.f));
            }

            for (auto &children = hierarchy->Children; 
                auto &child : children)
            {
                auto &transform = registry.get<TransformComponent>(child);
                UpdateTransform(child, registry, transform);
            }
        }
    }

    Scene::Scene()
    {
        using hs = entt::hashed_string;
        entt::meta_factory<MeshModel>{}.type(hs{"Model"}, "Model").base<Asset>();
        entt::meta_factory<Sound>{}.type(hs{"Sound"}, "Sound").base<Asset>();
        entt::meta_factory<Transform>{}.type(hs{"Transform"}, "Transform");

        m_Registry.on_destroy<Hierarchy>().connect<&Scene::OnEntityDestroyed>(*this);
    }

    Scene::~Scene()
    {
        m_Registry.on_destroy<Hierarchy>().disconnect<&Scene::OnEntityDestroyed>(*this);

        Clear();
    }

    void Scene::Clear()
    {
        m_Registry.clear();
    }

    void Scene::Save()
    {
        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.sasset";
        const bool        result    = Save(fileScene);

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

        entt::snapshot{m_Registry}.get<entt::entity>(serializer).ALL_COMPONENTS(serializer);

        file.close();

        return true;
    }

    void Scene::Load()
    {
        Clear();

        const std::string fileScene = DEFAULT_ASSETS_DIRECTORY + "Scenes/Scene.sasset";
        const bool        result    = Load(fileScene);

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

        m_Registry.clear();

        entt::snapshot_loader{m_Registry}.get<entt::entity>(serializer).ALL_COMPONENTS(serializer);

        file.close();

        return true;
    }

    void Scene::Update(const float deltaTime)
    {
        PROFILE_FUNCTION();

        auto view = m_Registry.view<MeshComponent, TransformComponent>();
        for (auto &&[entity, meshComponent, transform] : view.each())
        {
            if (meshComponent.Mesh; meshComponent.Mesh->IsDirty || transform.IsDirty())
            {
                meshComponent.Mesh->UpdateTransform(transform.Matrix());
                meshComponent.Mesh->IsDirty = false;
            }
        }

        if (!m_IsPaused)
        {
            auto rigidbodyGroup    = m_Registry.group<RigidbodyComponent>(entt::get<TransformComponent>);

            for (auto entity : rigidbodyGroup)
            {
                const auto &[transform, rigidbody] = rigidbodyGroup.get<TransformComponent, RigidbodyComponent>(entity);
                transform.SetPosition(rigidbody.Rigidbody->GetPosition());
                transform.SetRotation(rigidbody.Rigidbody->GetRotation());
            }

            const auto& scriptComponentView = m_Registry.view<ScriptComponent>();
            for (const auto& [entity, script] : scriptComponentView.each())
            {
                script.Update(deltaTime);
            }
        }
        
        for (const auto &transformView = m_Registry.view<TransformComponent>();  
            const auto &[entity, transform] : transformView.each())
        {
            if (transform.IsDirty())
            {
                UpdateTransform(entity, m_Registry, transform);
                transform.SetDirty(false);
            }
        }


        for (const auto& listenerView = m_Registry.view<AudioListenerComponent, TransformComponent>();
            const auto& [entity, listener, transform] : listenerView.each())
        {

        }
    }

    void Scene::StartScene()
    {
        auto viewRigidbodies = m_Registry.view<TransformComponent, RigidbodyComponent>();
        auto lightType       = entt::resolve<LightComponent>();
        
        for (auto&& [entity, transform, rigidbody] : viewRigidbodies.each())
        {
            rigidbody.Rigidbody->SetPosition(transform.GetPosition());
            rigidbody.Rigidbody->SetRotation(transform.GetRotation());

            const glm::vec3 position = rigidbody.Rigidbody->GetPosition();

            if (auto collision = m_Registry.try_get<HeroPhysics::BoxCollision>(entity); collision)
            {
                auto collisionPtr = std::make_shared<HeroPhysics::BoxCollision>(*collision);
                rigidbody.Rigidbody->SetCollisionShape(std::move(collisionPtr));
            }
            if (auto collision = m_Registry.try_get<HeroPhysics::SphereCollision>(entity); collision)
            {
                auto collisionPtr = std::make_shared<HeroPhysics::SphereCollision>(*collision);
                rigidbody.Rigidbody->SetCollisionShape(std::move(collisionPtr));
            }
            if (auto *storage = m_Registry.storage(lightType.info().hash()))
            {
                Log::Info("Success0");

                if (storage->contains(entity))
                {
                    Log::Info("Success");
                    void *componentPtr = storage->value(entity);
                }
            }
        }

        const auto scriptView = m_Registry.view<ScriptComponent>();
        for (auto&& [entity, scriptComponent] : scriptView.each())
        {
            scriptComponent.Reload();
            scriptComponent.BeginPlay(Entity{entity, this});
        }

        m_IsPaused = false;
        MEngine::Get().GetPhysicsSytem()->SetPause(false);
    }
    
    void Scene::StopScene()
    {
        m_IsPaused = true;

        MEngine::Get().GetPhysicsSytem()->SetPause(true);
        MEngine::Get().GetAudioSystem().StopAll();
        
        for (const auto& scriptView = m_Registry.view<ScriptComponent>();
            const auto& [entity, scriptComponent] : scriptView.each())
        {
            scriptComponent.EndPlay();
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

    void Scene::OnEntityDestroyed(entt::registry &registry, entt::entity entity)
    {
        auto *hierarchy = registry.try_get<Hierarchy>(entity);
        if (!hierarchy) return;

        if (hierarchy->Parent != entt::null)
        {
            if (auto *parent = registry.try_get<Hierarchy>(hierarchy->Parent))
                std::erase(parent->Children, entity);
        }

        const auto &children = hierarchy->Children;
        for (entt::entity c : children)
        {
            if (registry.valid(c))
            {
                registry.destroy(c);
            }
        }
    }

    template<>
    void Scene::RemoveComponent<MeshComponent>(Entity inEntity)
    {
        m_Registry.remove<MeshComponent>(inEntity);
    }

} // namespace MamontEngine