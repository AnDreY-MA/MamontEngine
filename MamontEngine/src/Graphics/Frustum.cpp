#include "Frustum.h"

namespace MamontEngine
{
    FrustumPlane ExtractFrustumPlanes(const glm::mat4& viewProj)
    {
        const glm::mat4 m = glm::transpose(viewProj);
        FrustumPlane result{};
        result.Planes[0] = m[3] + m[0]; //Left
        result.Planes[1] = m[3] - m[0]; //Right
        result.Planes[2] = m[3] + m[1]; //Bottom
        result.Planes[3] = m[3] - m[1]; //Top
        result.Planes[4] = m[2]; //Near
        result.Planes[5] = m[3] - m[2]; //Far

        for (auto& plane : result.Planes)
        {
            const glm::vec3 n(plane.x, plane.y, plane.z);
            const float     len = glm::length(n);
            if (len > 0.0f)
            {
                plane /= len;
            }
        }

        return result;
    }

    bool IsNodeVisible(const FrustumPlane &inFrustumPlane, const glm::mat4 &inTransform, const SphereBounds &inBoundSphere)
    {
        const glm::vec3 center = glm::vec3(inTransform * glm::vec4(inBoundSphere.Origin, 1.0f));
        const float     radius = inBoundSphere.Radius * glm::length(glm::vec3(inTransform[0]));

        for (const auto &plane : inFrustumPlane.Planes)
        {
            const float distance = glm::dot(glm::vec4(center, 1.0f), plane);
            if (distance < -radius)
            {
                return false;
            }
        }

        return true;
    }
}