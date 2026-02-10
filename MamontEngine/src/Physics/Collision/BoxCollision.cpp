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
	}
}