#pragma once

#include <entt/entt.hpp>
#include "Core/UID.h"
#include "Core/Camera.h"
#include "Utils/Serializer.h"
#include "ECS/Components/MeshComponent.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>

namespace MamontEngine
{
    class Entity;
    struct VkContextDevice;

	class Scene
    {
    public:
        // Scene() = default;
        explicit Scene();

        ~Scene();

        void Init();

        void Save();
        void Load();

        Entity CreateEntity(std::string_view inName = std::string_view());

        void Update();

        template <typename T>
        void RemoveComponent(Entity inEntity)
        {
            m_Registry.remove<T>(inEntity);
        }
        template <>
        void RemoveComponent<MeshComponent>(Entity inEntity);

        void DestroyEntity(Entity inEntity);

        entt::registry &GetRegistry()
        {
            return m_Registry;
        }

        const entt::registry &GetRegistry() const
        {
            return m_Registry;
        }
        Entity        GetEntity(UID Id);

    private:
        Entity CreateEntity(UID inId, std::string_view inName);

        void Clear();

        bool Save(std::string_view inFile);

        bool Load(std::string_view inFile);

    private:
        entt::registry m_Registry;

        friend class Entity;
        friend class SceneHierarchyPanel;

        template <typename Archive>
        friend void save(Archive &archive, const Scene &scene)
        {
        }

        template <typename Archive>
        friend void load(Archive& archive, Scene& scene)
        {

        }
    
	};

} // namespace MamontEngine
