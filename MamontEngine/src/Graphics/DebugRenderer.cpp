#include "Graphics/DebugRenderer.h"
#include <Utils/Profile.h>

namespace MamontEngine
{
    namespace DebugRenderer
    {
        std::vector<LineInfo> g_LineList;

        void DrawLine(const glm::vec3& inStart, const glm::vec3& inEnd, const Color& inColor, float inWidth)
        {

        }

        void Draw(const AABB &inBox, const Color &inColor, float width)
        {
            PROFILE_FUNCTION();

            const glm::vec3 uuu = inBox.Max;
            const glm::vec3 lll = inBox.Min;

            const glm::vec3 ull = glm::vec3(uuu.x, lll.y, lll.z);
            const glm::vec3 uul = glm::vec3(uuu.x, uuu.y, lll.z);
            const glm::vec3 ulu = glm::vec3(uuu.x, lll.y, uuu.z);

            const glm::vec3 luu = glm::vec3(lll.x, uuu.y, uuu.z);
            const glm::vec3 llu = glm::vec3(lll.x, lll.y, uuu.z);
            const glm::vec3 lul = glm::vec3(lll.x, uuu.y, lll.z);

            DrawLine(luu, uuu, inColor, width);
            DrawLine(lul, uul, inColor, width);
            DrawLine(llu, ulu, inColor, width);
            DrawLine(lll, ull, inColor, width);
            DrawLine(lul, lll, inColor, width);
            DrawLine(uul, ull, inColor, width);
            DrawLine(luu, llu, inColor, width);
            DrawLine(uuu, ulu, inColor, width);
            DrawLine(lll, llu, inColor, width);
            DrawLine(ull, ulu, inColor, width);
            DrawLine(lul, luu, inColor, width);
            DrawLine(uul, uuu, inColor, width);
        }

        void DrawBox(const glm::vec3 &inCenter, const glm::vec3 &inExtent, const glm::vec3 &inRotation, const Color &inColor)
        {
        
        }
    } // namespace DebugRenderer
} // namespace MamontEngine
