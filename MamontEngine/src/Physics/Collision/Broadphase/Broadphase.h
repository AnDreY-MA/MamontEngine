#pragma once


namespace MamontEngine
{
    namespace HeroPhysics
    {
        class Rigidbody;

        struct CollisionPair
        {
            Rigidbody *Object1;
            Rigidbody *Object2;
        };

        class Broadphase
        {
        public:
            Broadphase() = default;
            ~Broadphase() = default;

            virtual void FindCollisionPairs(Rigidbody* inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount) = 0;
        };
    }
}