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
#include "Graphics/Resources/AssetManager.h"
#include "Components/RigidbodyComponent.h"
#include "Physics/Collision/BoxCollision.h"
#include "Physics/Body/Rigidbody.h"
#include "Core/Engine.h"

META_INIT(void) 
IMPLEMENT_META_INIT(void)
{
    META_TYPE(float);
    META_TYPE(bool);
    META_TYPE(std::string);
    META_TYPE(entt::entity);
    META_TYPE(MamontEngine::Transform);
    META_TYPE(MamontEngine::Color);
    META_TYPE(MamontEngine::HeroPhysics::Rigidbody);
}
FINISH_REFLECT()

META_INIT(glm);

IMPLEMENT_META_INIT(glm)
{
    META_TYPE(glm::vec3);
}
FINISH_REFLECT()

#define ALL_COMPONENTS(serializer) get<IDComponent>(serializer).get<TagComponent>(serializer).get<TransformComponent>(serializer) \
        .get<MeshComponent>(serializer).get<DirectionLightComponent>(serializer).get<RigidbodyComponent>(serializer).get<HeroPhysics::BoxCollision>(serializer)


namespace MamontEngine
{
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

    Scene::Scene()
    {
        using hs = entt::hashed_string;
        entt::meta_factory<MeshModel>{}.type(hs{"Model"}, "Model").base<Asset>();
        entt::meta_factory<Transform>{}.type(hs{"Transform"}, "Transform");
    }

    Scene::~Scene()
    {
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
            if (meshComponent.Mesh)
            {
                meshComponent.Mesh->UpdateTransform(transform.Matrix());
            }
        }
    }

    void Scene::StartScene()
    {
        auto viewRigidbodies = m_Registry.view<TransformComponent>();
        viewRigidbodies.each([&](auto entity, TransformComponent& transform) {
                    auto rigidbodyComponent = m_Registry.try_get<RigidbodyComponent>(entity);
                    if (rigidbodyComponent)
                    {
                        rigidbodyComponent->Rigidbody->SetPosition(transform.Transform.Position);
                        rigidbodyComponent->Rigidbody->SetRotation(transform.Transform.Rotation);
                    }
            });
/*        for (auto [entity, transform, rigidbody] : viewRigidbodies.each())
        {
            rigidbody.Rigidbody->SetPosition(transform.Transform.Position);
            rigidbody.Rigidbody->SetRotation(transform.Transform.Rotation);

            const glm::vec3 position = rigidbody.Rigidbody->GetPosition();
            Log::Info("Rigidbody position: {} {} {}", position.x, position.y, position.z);

           /* if (auto collision = m_Registry.try_get<HeroPhysics::BoxCollision>(entity); collision)
            {
                auto collisionPtr = std::make_shared<HeroPhysics::BoxCollision>(*collision);
                rigidbody.Rigidbody->SetCollisionShape(std::move(collisionPtr));
            }

        }*/

        MamontEngine::MEngine::Get().GetPhysicsSytem()->SetPause(false);
    }
    
    void Scene::StopScene()
    {
        MamontEngine::MEngine::Get().GetPhysicsSytem()->SetPause(true);
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