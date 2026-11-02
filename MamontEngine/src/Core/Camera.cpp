#include "Camera.h"
#include "Core/InputSystem.h"
#include "Core/Log.h"
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

namespace MamontEngine
{
    glm::mat4 Camera::GetViewMatrix() const
    {
        const glm::mat4  cameraTranslation = glm::translate(glm::mat4(1.f), m_Position);
        const glm::mat4 cameraRotation    = GetRotationMatrix();
        return glm::inverse(cameraTranslation * cameraRotation);
    }

    glm::mat4 Camera::GetRotationMatrix() const
    {
        const glm::quat pitchRotation = glm::angleAxis(m_Pitch, glm::vec3{1.f, 0.f, 0.f});
        const glm::quat yawRotation   = glm::angleAxis(m_Yaw, glm::vec3{0.f, -1.f, 0.f});

        return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
    }

    void Camera::Rotate(const glm::vec2& inMotion)
    {
        const float dx = inMotion.x * m_MouseSensitivity;
        const float dy = inMotion.y * m_MouseSensitivity;
        m_Pitch -= dy;
        m_Yaw += dx;
    }

    void Camera::Update(float inDeltaTime)
    {
        const auto &input      = InputSystem::Get();
        const bool  rightMouse = input.IsMousePressed(KeyMouse::Right);
        m_IsMotion             = rightMouse;

        if (!m_IsMotion)
            return;

        Rotate(input.GetMouseMotion());

        const auto wheel = input.GetMouseWheel();
        // m_MovementSpeed += glm::clamp(wheel, 0.1f, wheel);

        if (input.IsKeyDown(KeyCode::w))
        {
            m_Velocity.z = -1.f;
        }
        if (input.IsKeyDown(KeyCode::s))
        {
            m_Velocity.z = 1.f;
        }
        if (input.IsKeyDown(KeyCode::d))
        {
            m_Velocity.x = 1.f;
        }
        if (input.IsKeyDown(KeyCode::a))
        {
            m_Velocity.x = -1.f;
        }
        if (input.IsKeyDown(KeyCode::e))
        {
            m_Velocity.y = 1.f;
        }
        if (input.IsKeyDown(KeyCode::q))
        {
            m_Velocity.y = -1.f;
        }

        const glm::mat4 cameraRotation = GetRotationMatrix();
        m_Position += glm::vec3(cameraRotation * glm::vec4(m_Velocity * m_MovementSpeed * inDeltaTime, 0.f));
        m_Velocity = glm::vec3(0.f);
    }

    void Camera::UpdateProjection(const VkExtent2D &inWindowExtent)
    {
        m_AspectRatio = (float)inWindowExtent.width / (float)inWindowExtent.height;
        m_Projection  = glm::perspectiveRH_ZO(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        m_Projection[1][1] *= -1;

        m_Frustum = ExtractFrustumPlanes(m_Projection * GetViewMatrix());
    }
} // namespace MamontEngine
