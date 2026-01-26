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

        glm::vec3 ToVector3() const
        {
            return glm::vec3(R, G, B);
        }

        float *Data()
        {
            return Componnts;
        }

        static const Color WHITE;
        static const Color BLACK;
        static const Color RED;
        static const Color GREEN;
        static const Color BLUE;

	};
}