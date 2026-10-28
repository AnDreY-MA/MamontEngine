#pragma once

#include <cereal/cereal.hpp>
#include <cereal/types/vector.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MamontEngine
{
    constexpr glm::vec3 FORWARD_VECTOR = glm::vec3(0.f, 0.f, 1.f);
    constexpr glm::vec3 UP_VECTOR      = glm::vec3(0.f, 1.f, 0.f);
    constexpr glm::vec3 RIGHT_VECTOR   = glm::vec3(1.f, 0.f, 0.f);

    struct Transform
    {
        glm::vec3 Position{glm::vec3(0.f)};
        glm::quat Rotation{glm::quat(1.0f, 0.0f, 0.0f, 0.0f)};
        glm::vec3 Scale{glm::vec3(1.f)};

        bool IsDirty{true};

        glm::mat4 Matrix() const
        {
            return glm::translate(glm::mat4(1.0f), Position) * glm::mat4_cast(Rotation)
                   * glm::scale(glm::mat4(1.0f), Scale);
        }

        void SetMatrix(const glm::mat4 inMatrix)
        {
            glm::vec3 skew = glm::vec3(1.f);
            glm::vec4 perspective = glm::vec4(1.0f);
            glm::decompose(inMatrix, Scale, Rotation, Position, skew, perspective);
        }

        bool operator==(const Transform &other) const
        {
            return Position == other.Position && Rotation == other.Rotation && Scale == other.Scale;
        }
        bool operator!=(const Transform &other) const
        {
            return !(*this == other);
        }

        inline glm::vec3 GetForwardVector() const
        {
            return Rotation * glm::vec3(0.f, 0.f, 1.0f);
        }
        inline glm::vec3 GetUpVector() const
        {
            return Rotation * UP_VECTOR;
        }
        inline glm::vec3 GetRightVector() const
        {
            return Rotation * RIGHT_VECTOR;
        }
    };


} // namespace MamontEngine

