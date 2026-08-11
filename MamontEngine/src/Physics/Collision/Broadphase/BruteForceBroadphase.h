#pragma once

#include "Physics/Collision/Broadphase/Broadphase.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class BruteForceBroadphase : public Broadphase
        {
        public:
            BruteForceBroadphase() = default;
            ~BruteForceBroadphase() = default;

            virtual void FindCollisionPairs(const std::vector<Rigidbody *> &inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount) override;
        };
    }
} // namespace MamontEngine
