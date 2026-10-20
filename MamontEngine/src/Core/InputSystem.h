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

        void HandleEvents();

        void ResetPressed();

        inline bool IsKeyPressed(KeyCode key) const
        {
            return m_KeyPressed[int(key)];
        }

        inline bool IsMouseClicked(KeyMouse key) const
        {
            return m_MouseClicked[int(key)];
        }

        void EnableDebug(bool inValue)
        {
            m_DebugEnabled = inValue;
        }

    private:
        static bool OnInputEvent(void *v, SDL_Event *event);

        inline void SetKeyPressed(KeyCode key, bool value) { m_KeyPressed[int(key)] = value; } 
        inline void SetKeyHeld(KeyCode key, bool value) { m_KeyHeld[int(key)] = value; } 

        inline void SetMouseClicked(KeyMouse key, bool value) { m_MouseClicked[int(key)] = value; } 


    private:
        bool m_KeyPressed[MAX_KEYS];
        std::array<bool, MAX_KEYS> m_KeyHeld;

        std::array<bool, MAX_KEYS> m_MouseClicked;

        bool m_DebugEnabled{false};
		
	};
}