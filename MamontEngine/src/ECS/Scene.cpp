#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "Graphics/Mesh.h"
#include <Utils/Loader.h>
#include "Graphics/Model.h"
#include "Core/ContextDevice.h"


namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Scene::Scene(const std::shared_ptr<SceneRenderer> &inSceneRenderer) 
        : m_SceneRenderer(inSceneRenderer)
    {
        
    }

    Scene::~Scene()
    {
        Clear();
    }

    void Scene::Init(VkContextDevice &inContextDevice)
    {
        {
            const std::string structurePath = {RootDirectories + "/MamontEngine/assets/monkey.glb"};
            std::shared_ptr<MeshModel> startModel = std::make_shared<MeshModel>(inContextDevice, structurePath);
            assert(startModel);

            auto entity = CreateEntity("House");
            entity.AddComponent<MeshComponent>(std::move(startModel));
        }

        {
            const std::string cubePath = {RootDirectories + "/MamontEngine/assets/monkeyHD.glb"};
            std::shared_ptr<MeshModel> cubeModel = std::make_shared<MeshModel>(inContextDevice, cubePath);
            assert(cubeModel);

            auto cubeEntity = CreateEntity("Cube");
            cubeEntity.AddComponent<MeshComponent>(std::move(cubeModel));
        }
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
        const std::string fileScene = RootDirectories + "/Scene.json";
        Serializer::SaveToFile(m_Registry, fileScene);
    }

    void Scene::Load()
    {
        Clear();
        const std::string fileScene = RootDirectories + "/Scene.json";
        Serializer::LoadFromFile(this, fileScene);
    }

    void Scene::Update()
    {
        const auto meshes = m_Registry.view<MeshComponent, TransformComponent>();
        for (const auto &&[entity, meshComponent, transform] : meshes.each())
        {
            if (meshComponent.Mesh)
            {
                meshComponent.Mesh->UpdateTransform(transform.GetTransform(), transform.Translation, transform.Rotation, transform.Scale);
            }

            if (meshComponent.Dirty == true)
            {
                m_SceneRenderer->SubmitMesh(meshComponent);
                meshComponent.Dirty = false;
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

        return entity;
    }

    template<>
    void Scene::RemoveComponent<MeshComponent>(Entity inEntity)
    {
        MeshComponent component = inEntity.GetComponent<MeshComponent>();
        m_SceneRenderer->RemoveMeshComponent(component);
        m_Registry.remove<MeshComponent>(inEntity);
    }

} // namespace MamontEngine
