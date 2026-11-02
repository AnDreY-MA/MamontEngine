#pragma once

#include <glm/glm.hpp>

namespace MamontEngine
{
    class Node;
    struct SphereBounds;

	struct FrustumPlane
	{
        std::array<glm::vec4, 6> Planes{};
	};

	FrustumPlane ExtractFrustumPlanes(const glm::mat4 &viewProj);

	bool IsNodeVisible(const FrustumPlane& inFrustumPlane, const glm::mat4 &inTransform, const SphereBounds &inBoundSphere);

}