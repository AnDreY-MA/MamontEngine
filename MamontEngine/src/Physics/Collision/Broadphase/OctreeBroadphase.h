#pragma once

#include "Physics/Collision/Broadphase/Broadphase.h"
#include "Math/AABB.h"

namespace MamontEngine
{
	namespace HeroPhysics
	{
		class OctreeBroadphase final : public Broadphase
		{
        public:
            void FindCollisionPairs(const std::vector<Rigidbody *> &inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount);

        private:
            struct OctreeNode
            {
                uint32_t    ChildCount{0};
                uint32_t    PhysicsObjectCount{0};
                OctreeNode *Children;
                Rigidbody **Objects;

                AABB Bound;
            };

            uint32_t m_LeafCount{0};

            OctreeNode   m_RootNode;
            OctreeNode **m_Leaves;

        private:
            void Divide(OctreeNode &division, size_t iteration);
		};
	}
}