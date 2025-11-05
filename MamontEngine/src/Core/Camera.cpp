#include "Camera.h"

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

    void Camera::Rotate(const SDL_MouseMotionEvent &inMotion)
    {
        const float dx = inMotion.xrel * m_MouseSensitivity;
        const float dy = inMotion.yrel * m_MouseSensitivity;
        m_Pitch -= dy;
        m_Yaw += dx;

       /* const float pitchLimit = glm::radians(89.0f);
        m_Pitch                = glm::clamp(m_Pitch, -pitchLimit, pitchLimit);*/
    }
    
    void Camera::ProccessEvent(SDL_Event &inEvent)
    {
        if (inEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN && inEvent.button.button == SDL_BUTTON_RIGHT)
        {
            m_IsRotating = true;
            m_IsMotion   = true;
        }

        if (inEvent.type == SDL_EVENT_MOUSE_BUTTON_UP && inEvent.button.button == SDL_BUTTON_RIGHT)
        {
            m_IsRotating = false;
            m_IsMotion   = false;
        }

        if (inEvent.type == SDL_EVENT_MOUSE_MOTION && m_IsRotating)
        {
            Rotate(inEvent.motion);
        }

        if (m_IsMotion)
        {
            if (inEvent.type == SDL_EVENT_MOUSE_WHEEL)
            {
                m_MovementSpeed += inEvent.wheel.y;
            }
        }

        if (inEvent.type == SDL_EVENT_KEY_DOWN)
        {
            if (inEvent.key.key == SDLK_E)
            {
                m_Velocity.y = 1;
            }
            if (inEvent.key.key == SDLK_Q)
            {
                m_Velocity.y = -1;
            }
            if (inEvent.key.key == SDLK_W)
            {
                m_Velocity.z = -1;
            }
            if (inEvent.key.key == SDLK_S)
            {
                m_Velocity.z = 1;
            }
            if (inEvent.key.key == SDLK_A)
            {
                m_Velocity.x = -1;
            }
            if (inEvent.key.key == SDLK_D)
            {
                m_Velocity.x = 1;
            }
        }

        if (inEvent.type == SDL_EVENT_KEY_UP)
        {
            if (inEvent.key.key == SDLK_E)
            {
                m_Velocity.y = 0;
            }
            if (inEvent.key.key == SDLK_Q)
            {
                m_Velocity.y = 0;
            }
            if (inEvent.key.key == SDLK_W)
            {
                m_Velocity.z = 0 ;
            }
            if (inEvent.key.key == SDLK_S)
            {
                m_Velocity.z = 0;
            }
            if (inEvent.key.key == SDLK_A)
            {
                m_Velocity.x = 0;
            }
            if (inEvent.key.key == SDLK_D)
            {
                m_Velocity.x = 0;
            }
        }
    }

    void Camera::Update(float inDeltaTime)
    {
        const glm::mat4 cameraRotation = GetRotationMatrix();
        m_Position += glm::vec3(cameraRotation * glm::vec4(m_Velocity * m_MovementSpeed * inDeltaTime, 0.f));
    }

    void Camera::UpdateProjection(const VkExtent2D &inWindowExtent)
    {
        m_AspectRatio = (float)inWindowExtent.width / (float)inWindowExtent.height;
        m_Projection  = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_NearClip, m_FarClip);
        //m_Projection = glm::perspective(glm::radians(m_FOV), m_AspectRatio, m_FarClip, m_NearClip);
        m_Projection[1][1] *= -1;

    }
} // namespace MamontEngine
