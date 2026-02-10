#include <cereal/access.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Math/Color.h"

namespace cereal
{
    template <typename Archive>
    void save(Archive &archive, const glm::vec3 &vector)
    {
        archive(vector.x, vector.y, vector.z);
    }

    template <typename Archive>
    void load(Archive &archive, glm::vec3 &vector)
    {
        archive(vector.x, vector.y, vector.z);
    }

    template <typename Archive>
    void save(Archive &archive, const glm::quat &quat)
    {
        archive(quat.x, quat.y, quat.z);
    }

    template <typename Archive>
    void load(Archive &archive, glm::quat &quat)
    {
        archive(quat.x, quat.y, quat.z);
    }

    template <typename Archive>
    void serialize(Archive& archive, MamontEngine::Color& color)
    {
        archive(cereal::make_nvp("R", color.R), cereal::make_nvp("G", color.G), cereal::make_nvp("B", color.B), cereal::make_nvp("A", color.A));
    }
}