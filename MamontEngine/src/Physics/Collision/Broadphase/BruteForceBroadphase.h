#pragma once

#include "Physics/Collision/Broadphase/Broadphase.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class BruteForceBroadphase final : public Broadphase
        {
            BruteForceBroadphase() = default;
            ~BruteForceBroadphase() = default;

            virtual void FindCollisionPairs(Rigidbody *inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount) override;
        };
    }
} // namespace MamontEngine
