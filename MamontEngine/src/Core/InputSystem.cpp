#include "Core/InputSystem.h"
#include <SDL3/SDL.h>
#include "Core/Log.h"

namespace
{
    [[nodiscard]] MamontEngine::KeyCode ToKeyCode(const SDL_Keycode keycode)
    {
        using enum MamontEngine::KeyCode;

        switch (static_cast<SDL_Keycode>(keycode))
        {
            case SDLK_UNKNOWN:
                return unknown;
            case SDLK_RETURN:
                return return_key;
            case SDLK_ESCAPE:
                return escape;
            case SDLK_BACKSPACE:
                return backspace;
            case SDLK_TAB:
                return tab;
            case SDLK_SPACE:
                return space;
            case SDLK_EXCLAIM:
                return exclaim;
            case SDLK_DBLAPOSTROPHE:
                return quotedbl;
            case SDLK_HASH:
                return hash;
            case SDLK_PERCENT:
                return percent;
            case SDLK_DOLLAR:
                return dollar;
            case SDLK_AMPERSAND:
                return ampersand;
            case SDLK_APOSTROPHE:
                return quote;
            case SDLK_LEFTPAREN:
                return left_paren;
            case SDLK_RIGHTPAREN:
                return right_paren;
            case SDLK_ASTERISK:
                return asterisk;
            case SDLK_PLUS:
                return plus;
            case SDLK_COMMA:
                return comma;
            case SDLK_MINUS:
                return minus;
            case SDLK_PERIOD:
                return period;
            case SDLK_SLASH:
                return slash;
            case SDLK_0:
                return _0;
            case SDLK_1:
                return _1;
            case SDLK_2:
                return _2;
            case SDLK_3:
                return _3;
            case SDLK_4:
                return _4;
            case SDLK_5:
                return _5;
            case SDLK_6:
                return _6;
            case SDLK_7:
                return _7;
            case SDLK_8:
                return _8;
            case SDLK_9:
                return _9;
            case SDLK_COLON:
                return colon;
            case SDLK_SEMICOLON:
                return semicolon;
            case SDLK_LESS:
                return less;
            case SDLK_EQUALS:
                return equals;
            case SDLK_GREATER:
                return greater;
            case SDLK_QUESTION:
                return question;
            case SDLK_AT:
                return at;
            case SDLK_LEFTBRACKET:
                return left_bracket;
            case SDLK_BACKSLASH:
                return backslash;
            case SDLK_RIGHTBRACKET:
                return right_bracket;
            case SDLK_CARET:
                return caret;
            case SDLK_UNDERSCORE:
                return underscore;
            case SDLK_GRAVE:
                return backquote;
            case SDLK_A:
                return a;
            case SDLK_B:
                return b;
            case SDLK_C:
                return c;
            case SDLK_D:
                return d;
            case SDLK_E:
                return e;
            case SDLK_F:
                return f;
            case SDLK_G:
                return g;
            case SDLK_H:
                return h;
            case SDLK_I:
                return i;
            case SDLK_J:
                return j;
            case SDLK_K:
                return k;
            case SDLK_L:
                return l;
            case SDLK_M:
                return m;
            case SDLK_N:
                return n;
            case SDLK_O:
                return o;
            case SDLK_P:
                return p;
            case SDLK_Q:
                return q;
            case SDLK_R:
                return r;
            case SDLK_S:
                return s;
            case SDLK_T:
                return t;
            case SDLK_U:
                return u;
            case SDLK_V:
                return v;
            case SDLK_W:
                return w;
            case SDLK_X:
                return x;
            case SDLK_Y:
                return y;
            case SDLK_Z:
                return z;
            case SDLK_CAPSLOCK:
                return capslock;
            case SDLK_F1:
                return f1;
            case SDLK_F2:
                return f2;
            case SDLK_F3:
                return f3;
            case SDLK_F4:
                return f4;
            case SDLK_F5:
                return f5;
            case SDLK_F6:
                return f6;
            case SDLK_F7:
                return f7;
            case SDLK_F8:
                return f8;
            case SDLK_F9:
                return f9;
            case SDLK_F10:
                return f10;
            case SDLK_F11:
                return f11;
            case SDLK_F12:
                return f12;
            default:
                break;
        }

        return unknown;
    }

    [[nodiscard]] std::string_view ToString(const MamontEngine::KeyCode key)
    {
        using namespace MamontEngine;

        switch (key)
        {
            case KeyCode::unknown:
                return "unknown";
            case KeyCode::return_key:
                return "return";
            case KeyCode::escape:
                return "escape";
            case KeyCode::backspace:
                return "backspace";
            case KeyCode::tab:
                return "tab";
            case KeyCode::space:
                return "space";

            case KeyCode::exclaim:
                return "!";
            case KeyCode::quotedbl:
                return "\"";
            case KeyCode::hash:
                return "#";
            case KeyCode::percent:
                return "%";
            case KeyCode::dollar:
                return "$";
            case KeyCode::ampersand:
                return "&";
            case KeyCode::quote:
                return "'";
            case KeyCode::left_paren:
                return "(";
            case KeyCode::right_paren:
                return ")";
            case KeyCode::asterisk:
                return "*";
            case KeyCode::plus:
                return "+";
            case KeyCode::comma:
                return ",";
            case KeyCode::minus:
                return "-";
            case KeyCode::period:
                return ".";
            case KeyCode::slash:
                return "/";

            case KeyCode::_0:
                return "0";
            case KeyCode::_1:
                return "1";
            case KeyCode::_2:
                return "2";
            case KeyCode::_3:
                return "3";
            case KeyCode::_4:
                return "4";
            case KeyCode::_5:
                return "5";
            case KeyCode::_6:
                return "6";
            case KeyCode::_7:
                return "7";
            case KeyCode::_8:
                return "8";
            case KeyCode::_9:
                return "9";

            case KeyCode::colon:
                return ":";
            case KeyCode::semicolon:
                return ";";
            case KeyCode::less:
                return "<";
            case KeyCode::equals:
                return "=";
            case KeyCode::greater:
                return ">";
            case KeyCode::question:
                return "?";
            case KeyCode::at:
                return "@";
            case KeyCode::left_bracket:
                return "[";
            case KeyCode::backslash:
                return "\\";
            case KeyCode::right_bracket:
                return "]";
            case KeyCode::caret:
                return "^";
            case KeyCode::underscore:
                return "_";
            case KeyCode::backquote:
                return "`";

            case KeyCode::a:
                return "a";
            case KeyCode::b:
                return "b";
            case KeyCode::c:
                return "c";
            case KeyCode::d:
                return "d";
            case KeyCode::e:
                return "e";
            case KeyCode::f:
                return "f";
            case KeyCode::g:
                return "g";
            case KeyCode::h:
                return "h";
            case KeyCode::i:
                return "i";
            case KeyCode::j:
                return "j";
            case KeyCode::k:
                return "k";
            case KeyCode::l:
                return "l";
            case KeyCode::m:
                return "m";
            case KeyCode::n:
                return "n";
            case KeyCode::o:
                return "o";
            case KeyCode::p:
                return "p";
            case KeyCode::q:
                return "q";
            case KeyCode::r:
                return "r";
            case KeyCode::s:
                return "s";
            case KeyCode::t:
                return "t";
            case KeyCode::u:
                return "u";
            case KeyCode::v:
                return "v";
            case KeyCode::w:
                return "w";
            case KeyCode::x:
                return "x";
            case KeyCode::y:
                return "y";
            case KeyCode::z:
                return "z";

            case KeyCode::capslock:
                return "capslock";

            case KeyCode::f1:
                return "f1";
            case KeyCode::f2:
                return "f2";
            case KeyCode::f3:
                return "f3";
            case KeyCode::f4:
                return "f4";
            case KeyCode::f5:
                return "f5";
            case KeyCode::f6:
                return "f6";
            case KeyCode::f7:
                return "f7";
            case KeyCode::f8:
                return "f8";
            case KeyCode::f9:
                return "f9";
            case KeyCode::f10:
                return "f10";
            case KeyCode::f11:
                return "f11";
            case KeyCode::f12:
                return "f12";

            case KeyCode::kp_0:
                return "kp_0";
            case KeyCode::kp_1:
                return "kp_1";
            case KeyCode::kp_2:
                return "kp_2";
            case KeyCode::kp_3:
                return "kp_3";
            case KeyCode::kp_4:
                return "kp_4";
            case KeyCode::kp_5:
                return "kp_5";
            case KeyCode::kp_6:
                return "kp_6";
            case KeyCode::kp_7:
                return "kp_7";
            case KeyCode::kp_8:
                return "kp_8";
            case KeyCode::kp_9:
                return "kp_9";
        }

        return "unknown";
    }

    [[nodiscard]] MamontEngine::KeyMouse ConvertMouseButton(const uint8_t button)
    {
        using namespace MamontEngine;
        switch (button)
        {
            case SDL_BUTTON_LEFT:
                return KeyMouse::Left;
            case SDL_BUTTON_RIGHT:
                return KeyMouse::Right;
            case SDL_BUTTON_MIDDLE:
                return KeyMouse::Middle;
        }
        return KeyMouse::Left;
    }

    [[nodiscard]] std::string_view ToString(const MamontEngine::KeyMouse button)
    {
        using namespace MamontEngine;
        using enum KeyMouse;

        switch (button)
        {
            case Left:
                return "Left";
            case Right:
                return "Right";
            case Middle:
                return "Middle";
        }
        return "";
    }
}

namespace MamontEngine
{
    InputSystem* s_InputSystem = nullptr;

    InputSystem::InputSystem()
    {
        s_InputSystem = this;

        SDL_AddEventWatch(&InputSystem::OnInputEvent, nullptr);
    }
    
    InputSystem::~InputSystem()
    {
        s_InputSystem = nullptr;
    }

    void InputSystem::ResetPressed()
    {
        memset(m_KeyPressed, 0, MAX_KEYS);
        std::memset(m_MouseClicked.data(), 0, MAX_BUTTONS);
    }
    
    InputSystem &InputSystem::Get()
    {
        return *s_InputSystem;
    }

    bool SDLCALL InputSystem::OnInputEvent(void * v, SDL_Event *event)
    {
        const auto type = event->type;

        if (type == SDL_EVENT_KEY_DOWN)
        {
            const auto keyCode = ToKeyCode(event->key.key);
            auto       stringCode = ToString(keyCode);

            if (s_InputSystem->m_DebugEnabled)
            {
                Log::Info("KeyDown: {}", stringCode);
            }

            InputSystem::Get().SetKeyPressed(keyCode, true);
        }

        if (type == SDL_EVENT_KEY_UP)
        {
            const auto keyCode    = ToKeyCode(event->key.key);
            auto       stringCode = ToString(keyCode);
            if (s_InputSystem->m_DebugEnabled)
            {
                Log::Info("KeyUp: {}", stringCode);
            }
            InputSystem::Get().SetKeyPressed(keyCode, false);
        }


        if (type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            const auto mouseCode    = ConvertMouseButton(event->button.button);
            if (s_InputSystem->m_DebugEnabled)
            {
                const auto buttonString = ToString(mouseCode);
                Log::Info("[Input]: mouse button: {}", buttonString);
            }
            InputSystem::Get().SetMouseClicked(mouseCode, true);
        }

        if (type == SDL_EVENT_MOUSE_BUTTON_UP)
        {
            const auto mouseCode    = ConvertMouseButton(event->button.button);
            if (s_InputSystem->m_DebugEnabled)
            {
                const auto buttonString = ToString(mouseCode);
                Log::Info("[Input]: mouse button up: {}", buttonString);
            }

            InputSystem::Get().SetMouseClicked(mouseCode, false);
        }

        return true;
    }
}