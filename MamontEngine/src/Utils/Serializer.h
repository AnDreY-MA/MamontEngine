#pragma once

#include <nlohmann/json.hpp>
#include <entt/entt.hpp>

namespace MamontEngine
{
    struct TransformComponent;
    class Scene;
    class UID;

    class Serializer final
    {
    public:
        static bool SaveToFile(const entt::registry &registry, const std::string &filename);
        static bool LoadFromFile(Scene *inScene, const std::string &filename);

        static nlohmann::json SaveScene(const entt::registry &inRegistry);
        static void           LoadScene(const nlohmann::json &inJsonFile, Scene* inScene);
        

    private:
        static nlohmann::json SerializeTransform(const TransformComponent &inTransformComponent);

        static std::string DeserializeTag(const nlohmann::json& inJson);

        static UID DeserializeID(const nlohmann::json &inJson);

        static TransformComponent DeserializeTransform(const nlohmann::json &inJson);

        static std::string DeserializeMesh(const nlohmann::json &inJson);


    };
}