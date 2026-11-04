#include "ECS/Components/TransformComponent.h"
#include "Core/Log.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(TransformComponent, "Transform Component")
    {
        meta.data<&TransformComponent::m_Transform, entt::as_ref_t>("Transform"_hs);
    }
    FINISH_REFLECT()

    void TransformComponent::SetDirty(bool value)
    {
        m_Transform.IsDirty = value;
    }

    void TransformComponent::SetWorldMatrix(const glm::mat4 &inMatrix)
    {
        m_WorldMatrix = inMatrix;
        SetDirty(true);
    }

    void TransformComponent::SetMatrix(const glm::mat4 &inMatrix)
    {
        m_Transform.SetMatrix(inMatrix);
        SetDirty(true);
        
    }

    glm::mat4 TransformComponent::GetLocalMatrix() const
    {
        return m_Transform.Matrix();
    }
} // namespace MamontEngine
