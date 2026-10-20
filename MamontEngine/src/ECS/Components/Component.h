#pragma once 

#include "Utils/MetaReflection.h"

namespace MamontEngine
{
	struct Component
	{
    private:
        REFLECT()
    public:
        Component() = default;
        virtual ~Component() = default;
	};
}