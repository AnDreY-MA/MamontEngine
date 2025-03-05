#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "Graphics/Mesh.h"
#include <Utils/Loader.h>

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    Scene::Scene(const std::shared_ptr<SceneRenderer> &inSceneRenderer) : m_SceneRenderer(inSceneRenderer)
    {
        
    }

    Scene::~Scene()
    {
        m_Registry.clear();
        m_Entities.clear();
    }

    void Scene::Init(VkContextDevice &inContextDevice)
    {
        const std::string structurePath = {RootDirectories + "/MamontEngine/assets/house2.glb"};
        auto              structureFile = loadGltf(inContextDevice, structurePath);
        assert(structureFile.has_value());

        auto entity = CreateEntity("House");
        entity.AddComponent<MeshComponent>(*structureFile);
    }

    void Scene::Update()
    {
        const auto meshes = m_Registry.view<MeshComponent, TransformComponent, TagComponent>();
        for (const auto &&[entity, meshComponent, transform, tag] : meshes.each())
        {
            {
                if (meshComponent.Mesh && !meshComponent.Mesh->Nodes.empty())
                {
                    for (auto &n : meshComponent.Mesh->Nodes)
                    {
                        n->LocalTransform = transform.GetTransform();
                        n->RefreshTransform({1.f});
                    }
                }
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
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag   = inName.empty() ? "Enity" : inName;
        entity.AddComponent<TransformComponent>();

        m_Entities.emplace(inId, entity);

        return entity;
    }

} // namespace MamontEngine
