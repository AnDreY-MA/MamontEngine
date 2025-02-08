#include "Camera.h"

#include "SDL3/SDL_events.h"
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_mouse.h>

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
    }
    
    void Camera::ProccessEvent(SDL_Event &inEvent)
    {
        if (inEvent.type == SDL_EVENT_MOUSE_BUTTON_DOWN && inEvent.button.button == SDL_BUTTON_RIGHT)
        {
            m_IsRotating = true;
        }

        if (inEvent.type == SDL_EVENT_MOUSE_BUTTON_UP && inEvent.button.button == SDL_BUTTON_RIGHT)
        {
            m_IsRotating = false;
        }

        if (inEvent.type == SDL_EVENT_MOUSE_MOTION && m_IsRotating)
        {
            Rotate(inEvent.motion);
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

    void Camera::Update()
    {
        const glm::mat4 cameraRotation = GetRotationMatrix();
        m_Position += glm::vec3(cameraRotation * glm::vec4(m_Velocity * 0.5f, 0.f));
    }
} // namespace MamontEngine
