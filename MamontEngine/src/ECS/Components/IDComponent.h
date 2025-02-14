#pragma once

#include "Core/UID.h"

namespace MamontEngine
{
	struct IDComponent
	{
        IDComponent() = default;
        IDComponent(const IDComponent &) = default;

		UID ID;
	};
}