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
            return m_Transform.Matrix();
        }

        void SetPosition(const glm::vec3& inPosition)
        {
            m_Transform.Position = inPosition;
            SetDirty(true);
        }

        void SetRotation(const glm::quat& inRotation)
        {
            m_Transform.Rotation = inRotation;
            SetDirty(true);
        }

        void SetRotation(const glm::vec3 &inScale)
        {
            m_Transform.Scale = inScale;
            SetDirty(true);
        }

        inline const glm::vec3& GetPosition() const { return m_Transform.Position; }
        inline const glm::quat& GetRotation() const { return m_Transform.Rotation; }
        inline const glm::vec3& GetScale() const { return m_Transform.Scale; }

        inline glm::vec3 GetForwardVector() const
        {
            return m_Transform.GetForwardVector();
        }

        inline void SetDirty(bool value) { m_Transform.IsDirty = value; }
        inline bool IsDirty() const { return m_Transform.IsDirty; }

    private:

        Transform m_Transform;

        friend class cereal::access;

        template <class Archive>
        void serialize(Archive& ar)
        {
            ar(m_Transform.Position, m_Transform.Rotation, m_Transform.Scale);
        }

        REFLECT()
    };
}