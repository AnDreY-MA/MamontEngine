#pragma once

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

        void UpdateProjection(const VkExtent2D &inWindowExtent);

        const glm::mat4& GetProjection() const
        {
            return m_Projection;
        }

        void SetVelocity(const glm::vec3& inVelocity)
        {
            m_Velocity = inVelocity;
        }

        void SetPosition(const glm::vec3& inPosition)
        {
            m_Position = inPosition;
        }

        float GetNearClip() const noexcept
        {
            return m_NearClip;
        }
        float GetFarClip() const noexcept
        {
            return m_FarClip;
        }

    private:
        void Rotate(const SDL_MouseMotionEvent& inMotion);

    private:
        glm::mat4 m_Projection;

        glm::vec3 m_Velocity;
        glm::vec3 m_Position;
        
        float     m_Pitch{0.f};
        float     m_Yaw{0.f};

        bool      m_IsRotating       = false;
        float     m_MouseSensitivity = 0.001f;

        float m_NearClip{0.1f};
        float m_FarClip{1000.f};

        float m_FOV{70.f};
	};
}