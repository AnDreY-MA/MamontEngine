#pragma once

#include "Math/AABB.h"
#include "Math/Color.h"

namespace MamontEngine
{
   struct LineInfo
    {
        glm::vec3 P1{0.f};
        glm::vec3 P2{0.f};
        Color     Color;

        LineInfo() = default;

        LineInfo(const glm::vec3 pos1, const glm::vec3 &pos2, const MamontEngine::Color &inColor) 
            : P1(pos1), P2(pos1), Color(inColor)
        {

        }
    };

    namespace DebugRenderer
    {
        void DrawLine(const glm::vec3 &inStart, const glm::vec3 &inEnd, const Color &inColor, float inWidth = 1.0f);

        void Draw(const AABB &inBox, const Color &inColor, float width = 1.0f);
        
        void DrawBox(const glm::vec3 &inCenter, const glm::vec3 &inExtent, const glm::vec3 &inRotation, const Color &inColor);
    } // namespace DebugRenderer
} // namespace MamontEngine
