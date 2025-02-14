#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"


namespace MamontEngine
{
    Entity Scene::CreateEntity(std::string_view inName)
    {
        return CreateEntity(UID(), inName);
    }

    void Scene::DestroyEntity(Entity inEntity)
    {
        m_Entities.erase(inEntity.GetID());
        m_Registry.destroy(inEntity);
    }

    Entity Scene::CreateEntity(UID inId, std::string_view inName)
    {
        Entity entity = {m_Registry.create(), this};
        entity.AddComponent<TransformComponent>();
        auto &tag = entity.AddComponent<TagComponent>();
        tag.Tag   = inName.empty() ? "Enity" : inName;

        m_Entities[inId] = entity;

        return entity;
    }

} // namespace MamontEngine
