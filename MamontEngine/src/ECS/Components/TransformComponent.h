#pragma once 

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace MamontEngine
{
	struct TransformComponent
	{
        glm::vec3 Translation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Rotation = {0.0f, 0.0f, 0.0f};
        glm::vec3 Scale = {1.0f, 1.0f, 1.0f};

		TransformComponent() = default;
        TransformComponent(const TransformComponent &) = default;
        TransformComponent(const glm::vec3 &inTranslation) : Translation(inTranslation)
        {
        }

        glm::mat4 GetTransform() const
        {
            const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), Rotation.x, {1, 0, 0}) * glm::rotate(glm::mat4(1.0f), Rotation.y, {0, 1, 0}) *
                                 glm::rotate(glm::mat4(1.0f), Rotation.z, {0, 0, 1});

            return glm::translate(glm::mat4(1.0f), Translation) * rotation * glm::scale(glm::mat4(1.0f), Scale);
        }
	};
}