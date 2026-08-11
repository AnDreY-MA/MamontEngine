#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/Broadphase/OctreeBroadphase.h"
#include "Physics/Collision/CollisionShape.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        void
        OctreeBroadphase::FindCollisionPairs(const std::vector<Rigidbody *> &inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount)
        {
            for (uint32_t i = 0; i < totalBodyCount; i++)
            {
                Rigidbody* current = inBodies[i];

                if (const auto shape = current->GetShape(); shape)
                {
                    // m_RootNode.Bound.Merge()
                    m_RootNode.Objects[m_RootNode.PhysicsObjectCount] = current;
                    m_RootNode.PhysicsObjectCount++;
                }
            }
        }

        void OctreeBroadphase::Divide(OctreeNode &division, size_t iteration)
        {
        }
    } // namespace HeroPhysics
} // namespace MamontEngine
