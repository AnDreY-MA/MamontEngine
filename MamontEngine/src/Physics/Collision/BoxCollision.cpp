#include "Physics/Collision/BoxCollision.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
	namespace HeroPhysics
	{
        IMPLEMENT_REFLECT_COMPONENT(BoxCollision, "BoxCollision Component")
        {
            meta.data<&BoxCollision::m_HalfExtent>(entt::hashed_string{"HalfExtent"}, "Half Extent");
		}
        FINISH_REFLECT()

		glm::mat3 BoxCollision::BuildInverseInertia(const float inMass) const
        {
            glm::mat3 inertia{glm::mat3(1.f)};
            
            return inertia;
        }

        std::vector<glm::vec3> BoxCollision::GetAxes(const glm::quat &inOrientation) const
        {
            std::vector<glm::vec3> axes;
            axes.resize(3);

            const glm::mat3 matOrientation = glm::mat3(inOrientation);
            axes[0]                          = (matOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
            axes[1]                  = (matOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
            axes[2]                          = (matOrientation * glm::vec3(0.0f, 0.0f, 1.0f));

            return axes;
        }

        std::vector<CollisionEdge> BoxCollision::GetEdges(const glm::mat4 &inTransform) const
        {
            std::vector<CollisionEdge> edges;

            return edges;
        }

	}
}