#pragma once

#include <variant>
#include "KeyCode.h"

namespace MamontEngine
{
	struct MouseMoveEventData
	{
        int X;
        int Y;
	};

	struct MouseScrollEventData
	{
        float XOffset;
        float YOffset;
	};

	struct WindowResizeEventData
	{
        int Width;
        int Height;
	};

	struct KeyboardEventData
	{
        enum class EType { Down, Up};

		EType Type;

        KeyCode KeyCode;
	};

	using EventData = std::variant<std::monostate,
		MouseMoveEventData, MouseScrollEventData, WindowResizeEventData, KeyboardEventData>;

	struct InputEvent
	{
        EventData Data;
	};
}