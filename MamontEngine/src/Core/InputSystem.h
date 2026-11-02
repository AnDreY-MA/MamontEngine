#pragma once

#include "Event/KeyCode.h"

namespace MamontEngine
{
    constexpr int MAX_KEYS = 1024;
    constexpr int MAX_BUTTONS = 32;

	class InputSystem
	{
    public:
        explicit InputSystem();
        ~InputSystem();

        static InputSystem &Get();

        void OnEvent(void *inEvent);

        void NewFrame();

        void ResetPressed();

        inline bool IsKeyDown(KeyCode key) const
        {
            return m_Keys[int(key)];
        }

        inline bool IsKeyPressed(KeyCode key) const
        {
            return m_Keys[int(key)] && !m_PreviousKeys[int(key)];
        }
        inline bool IsKeyReleased(KeyCode key) const
        {
            return !m_Keys[int(key)] && m_PreviousKeys[int(key)];
        }

        inline bool IsMousePressed(KeyMouse key) const
        {
            return m_MouseButton[int(key)];
        }

        inline bool IsMouseReleased(KeyMouse key) const
        {
            return !m_MouseButton[int(key)] && m_PreviousMouseButton[int(key)];
        }

        void EnableDebug(bool inValue)
        {
            m_DebugEnabled = inValue;
        }

        glm::vec2 GetMouseMotion() const
        {
            return m_MouseMotion;
        }

        float GetMouseWheel() const { return m_MouseWheel; }

    private:
        std::array<bool, MAX_KEYS> m_Keys;
        std::array<bool, MAX_KEYS> m_PreviousKeys;


        std::array<bool, MAX_BUTTONS> m_MouseButton;
        std::array<bool, MAX_BUTTONS> m_PreviousMouseButton;

        glm::vec2 m_MouseMotion{glm::vec2(0.f)};

        float m_MouseWheel{0.f};

        bool m_DebugEnabled{false};
		
	};
}