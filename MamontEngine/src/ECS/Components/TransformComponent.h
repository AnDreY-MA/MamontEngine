#pragma once 

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Math/Transform.h"
#include "ECS/Components/Component.h"
#include "Utils/MetaReflection.h"
#include "Graphics/Resources/Models/Model.h"

#include <cereal/cereal.hpp>

namespace MamontEngine
{
    struct TransformComponent
    {
        TransformComponent()                                = default;
        TransformComponent(const TransformComponent &other) = default;

        glm::mat4 Matrix() const
        {
            return Transform.Matrix();
        }

        Transform Transform;

        bool IsDirty{true};

    private:

        float TestValue = 1.f;

        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(Transform.Position, Transform.Rotation, Transform.Scale);
        }

        REFLECT()
    };
}