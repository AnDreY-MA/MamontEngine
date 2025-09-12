#include "Utils/Serializer.h"
#include "ECS/Components/IDComponent.h"
#include "ECS/Components/TagComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/MeshComponent.h"
#include "Graphics/Model.h"
#include "ECS/Scene.h"
#include "ECS/Entity.h"
#include <fstream>

#include "Core/Engine.h"

namespace MamontEngine
{
    constexpr std::string JSON_ENTITIES = "Entities";
    constexpr std::string JSON_COMPONENTS = "Components";
    constexpr std::string JSON_MESH = "Mesh";
    constexpr std::string JSON_TRANSFORM = "Transform";
    constexpr std::string JSON_TRANSFORM_TRANSLATION = "position";
    constexpr std::string JSON_TRANSFORM_ROTATION = "rotation";
    constexpr std::string JSON_TRANSFORM_SCALE = "scale";
    constexpr std::string JSON_TAG = "Tag";

    bool Serializer::SaveToFile(const entt::registry &registry, const std::string &filename)
    {
        try
        {
            nlohmann::json sceneJson = SaveScene(registry);
            std::ofstream file(filename);
            file << sceneJson.dump(4);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to save scene: " << e.what() << std::endl;
            return false;
        }
    }

    bool Serializer::LoadFromFile(Scene *inScene, const std::string &filename)
    {
        try
        {
            std::ifstream file(filename);
            if (!file.is_open())
            {
                fmt::println("File: {} is not open", filename);
                return false;
            }

            nlohmann::json sceneJson;
            file >> sceneJson;

            LoadScene(sceneJson, inScene);
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Failed to load scene: " << e.what() << std::endl;
            return false;
        }
    }


    nlohmann::json Serializer::SaveScene(const entt::registry &inRegistry)
    {
        nlohmann::json sceneJson;
        sceneJson[JSON_ENTITIES] = nlohmann::json::array();
        fmt::println("Serializer::SaveScene");
        auto view = inRegistry.view<entt::entity>();
        for (const auto& entity : view)
        {
            nlohmann::json entityJson;
            if (inRegistry.all_of<IDComponent>(entity))
            {
                const auto IdComponent = inRegistry.try_get<IDComponent>(entity);
                entityJson[JSON_COMPONENTS]["ID"] = (uint64_t)IdComponent->ID;
            }
            if (inRegistry.all_of<TagComponent>(entity))
            {
                const auto tagComponent = inRegistry.try_get<TagComponent>(entity);
                entityJson[JSON_COMPONENTS]["Tag"] = tagComponent->Tag.c_str();

            }
            if (inRegistry.all_of<TransformComponent>(entity))
            {
                const auto transform = inRegistry.try_get<TransformComponent>(entity);
                entityJson[JSON_COMPONENTS][JSON_TRANSFORM] = SerializeTransform(*transform);
            }
            if (inRegistry.all_of<MeshComponent>(entity))
            {
                const auto meshComponent                  = inRegistry.try_get<MeshComponent>(entity);
                entityJson[JSON_COMPONENTS][JSON_MESH] = meshComponent->Mesh->GetPathFile();
            }

            sceneJson[JSON_ENTITIES].push_back(entityJson);
        }

        return sceneJson;
        
    }

    void Serializer::LoadScene(const nlohmann::json &inJsonFile, Scene *inScene)
    {
        if (!inJsonFile.contains(JSON_ENTITIES))
        {
            fmt::println("Json file not contains entities");
            return;
        }

        const auto& contextDevice = MEngine::Get().GetContextDevice();

        const auto &entities = inJsonFile[JSON_ENTITIES];
        fmt::println("Json file size - {}", entities.size());

        for (const auto &entityJson : entities)
        {
            if (entityJson.contains(JSON_COMPONENTS))
            {
                const auto& jsonComponent = entityJson[JSON_COMPONENTS];
                fmt::println("jsonComponent size: {}", jsonComponent.size());

                const auto  id            = DeserializeID(jsonComponent);
                const std::string  tag           = DeserializeTag(jsonComponent);
                auto entity = inScene->CreateEntity(id, tag);

                const TransformComponent transformData = DeserializeTransform(jsonComponent); 
                auto& trasnform = entity.GetComponent<TransformComponent>();
                trasnform.Translation                  = transformData.Translation;
                trasnform.Rotation                     = transformData.Rotation;
                trasnform.Scale                        = transformData.Scale;

                const auto meshPath = DeserializeMesh(jsonComponent);
                if (meshPath != "")
                {
                    auto model = std::make_shared<MeshModel>(contextDevice, meshPath);
                    entity.AddComponent<MeshComponent>(std::move(model));
                }

            }
        }
    }

    nlohmann::json Serializer::SerializeTransform(const TransformComponent &inTransformComponent)
    {
        return {{JSON_TRANSFORM_TRANSLATION, {inTransformComponent.Translation.x, inTransformComponent.Translation.y, inTransformComponent.Translation.z}},
                {JSON_TRANSFORM_ROTATION, {inTransformComponent.Rotation.x, inTransformComponent.Rotation.y, inTransformComponent.Rotation.z}},
                {JSON_TRANSFORM_SCALE, {inTransformComponent.Scale.x, inTransformComponent.Scale.y, inTransformComponent.Scale.z}}};
    }
    
    std::string Serializer::DeserializeTag(const nlohmann::json &inJson)
    {
        std::string tag = std::string("");

        if (!inJson.contains(JSON_TAG))
        {
            fmt::println("jsonFile don't contains tag");
            return tag;
        }

        tag = inJson[JSON_TAG];

        return tag;
    }
    
    UID Serializer::DeserializeID(const nlohmann::json &inJson)
    {
        if (!inJson.contains("ID"))
        {
            return {};
        }

        UID Id{static_cast<uint64_t>(inJson["ID"])};

        return Id;
    }

    TransformComponent Serializer::DeserializeTransform(const nlohmann::json &inJson)
    {
        TransformComponent transform;
        if (!inJson.contains(JSON_TRANSFORM))
        {
            fmt::println("Json don't contains transform");
            return transform;
        }

        const auto& transforCol = inJson[JSON_TRANSFORM];

        if (transforCol.contains(JSON_TRANSFORM_TRANSLATION))
        {
            const auto &pos       = transforCol[JSON_TRANSFORM_TRANSLATION];
            transform.Translation = {(float)pos[0], (float)pos[1], (float)pos[2]};
        }
        if (transforCol.contains(JSON_TRANSFORM_ROTATION))
        {
            const auto &rot    = transforCol[JSON_TRANSFORM_ROTATION];
            transform.Rotation = {(float)rot[0], (float)rot[1], (float)rot[2]};
        }
        if (transforCol.contains(JSON_TRANSFORM_SCALE))
        {
            const auto &scale = transforCol[JSON_TRANSFORM_SCALE];
            transform.Scale = {(float)scale[0], (float)scale[1], (float)scale[2]};
        }

        return transform;

    }

    std::string Serializer::DeserializeMesh(const nlohmann::json &inJson)
    {
        std::string mesmPath = std::string("");
        if (!inJson.contains(JSON_MESH))
        {
            return mesmPath;
        }
        mesmPath = inJson[JSON_MESH];

        return mesmPath;
    }

}