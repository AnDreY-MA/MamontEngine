#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>

namespace MamontEngine
{
    constexpr glm::vec3 FORWARD_VECTOR = glm::vec3(1.f, 0.f, 0.f);
    constexpr glm::vec3 UP_VECTOR      = glm::vec3(0.f, 1.f, 0.f);
    constexpr glm::vec3 RIGHT_VECTOR   = glm::vec3(0.f, 0.f, 1.f);

    struct Transform
    {
        glm::vec3 Position{glm::vec3(0.f)};
        glm::quat Rotation{glm::vec3(0.f)};
        glm::vec3 Scale{glm::vec3(1.f)};

        glm::mat4 Matrix() const
        {
            glm::mat4 matrix{glm::mat4(1.0f)};

            matrix = glm::translate(matrix, Position);
            matrix = glm::rotate(matrix, glm::radians(Rotation.x), RIGHT_VECTOR);
            matrix = glm::rotate(matrix, glm::radians(Rotation.y), UP_VECTOR);
            matrix = glm::rotate(matrix, glm::radians(Rotation.z), FORWARD_VECTOR);
            matrix = glm::scale(matrix, Scale);

            return matrix;
        }

        bool operator==(const Transform &other) const
        {
            return Position == other.Position && Rotation == other.Rotation && Scale == other.Scale;
        }
        bool operator!=(const Transform &other) const
        {
            return Position != other.Position && Rotation != other.Rotation && Scale != other.Scale;
        }
    };


} // namespace MamontEngine

