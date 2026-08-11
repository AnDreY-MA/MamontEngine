#pragma once 

#include "Physics/Collision/Manifold.h"

namespace MamontEngine
{
	namespace HeroPhysics
	{
        struct CollisionPair;

		struct CollisionData
		{
            float Penetration{0};
            glm::vec3 Normal{glm::vec3(0.f)};
            glm::vec3 Point{glm::vec3(0.f)};
		};

		//bool CheckCollision(const CollisionPair *inPair, std::vector<ContactManifold> &outResults);
        bool CheckCollision(const CollisionPair *inPair, CollisionData *collisionData);
	}
}