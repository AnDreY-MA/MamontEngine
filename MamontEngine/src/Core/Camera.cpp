#include "Camera.h"

namespace MamontEngine
{
    glm::mat4 Camera::GetViewMatrix()
    {
        return glm::mat4();
    }
    glm::mat4 Camera::GetRotationMatrix()
    {
        return glm::mat4();
    }
    void Camera::ProccessEvent(SDL_Event &inEvent)
    {
    }
    void Camera::Update()
    {
        glm::mat4 cameraRotation = GetRotationMatrix();
        position += glm::vec3(cameraRotation * glm::vec4(m_Velocity * 0.5f, 0.f));
    }
} // namespace MamontEngine
