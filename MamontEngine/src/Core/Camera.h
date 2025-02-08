#pragma once

#include "SDL3/SDL_events.h"

namespace MamontEngine
{
	class Camera
	{
    public:
        Camera() = default;
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetRotationMatrix() const;

        void ProccessEvent(SDL_Event &inEvent);
        void Update();

        void SetVelocity(const glm::vec3& inVelocity)
        {
            m_Velocity = inVelocity;
        }

        void SetPosition(const glm::vec3& inPosition)
        {
            m_Position = inPosition;
        }

    private:
        void Rotate(const SDL_MouseMotionEvent& inMotion);

    private:
        glm::vec3 m_Velocity;
        glm::vec3 m_Position;
        float     m_Pitch{0.f};
        float     m_Yaw{0.f};
        bool      m_IsRotating       = false;
        float     m_MouseSensitivity = 0.001f;
	};
}