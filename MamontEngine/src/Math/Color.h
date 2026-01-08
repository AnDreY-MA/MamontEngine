#pragma once

#include "glm/glm.hpp"

namespace MamontEngine
{
	struct Color
	{
        union
        {
            struct
            {
                float R, G, B, A;
            };

            float Componnts[4] = {0.f, 0.f, 0.f, 1.f};
        };

        constexpr Color() : R(0.f), G(0.f), B(0.f), A(1.f)
        {
        }

        constexpr Color(float r, float g, float b, float a = 1.f) : R(a), G(g), B(b), A(a)
        {
        }

        glm::vec4 ToVector4() const
        {
            return glm::vec4(R, G, B, A);
        }

        float *Data()
        {
            return Componnts;
        }

        static constexpr Color WHITE(1.0f, 1.0f, 1.0f);
        static constexpr Color BLACK(0.0f, 0.0f, 0.0f);
        static constexpr Color GRAY(0.0f, 0.0f, 0.0f);

	};
}