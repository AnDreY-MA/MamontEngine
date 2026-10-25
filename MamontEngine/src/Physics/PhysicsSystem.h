#pragma once

#include <entt/entt.hpp>
#include "Collision/Broadphase/Broadphase.h"
#include "Collision/Manifold.h"
#include "Utils/MetaReflection.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
    class Rigidbody;
    
    struct PhysicsSettings
    {
        float TimeStep{1.f / 60.f};
        glm::vec3 Gravity{glm::vec3(0.f, -9.81f, 0.f)};

        uint32_t MaxBodyCount{4096};
    };

    constexpr int MAX_MANIFOLDS = 1000;

	class PhysicsSystem
	{
    public:
        PhysicsSystem(const PhysicsSettings &settings = {});
        ~PhysicsSystem();

        void Update(const float inDeltaTime);

        inline bool IsPaused() const { return m_IsPaused; }

        inline void SetPause(bool inValue) { m_IsPaused = inValue; }

        Rigidbody *CreateBody();

        void DestroyBody(Rigidbody *body);

    private:
        void UpdateRigidbodies();

        void UpdateRigidbody(Rigidbody *body, const float deltaTime);
        void IntegrateForce(Rigidbody *body, const float deltaTime);
        void IntegrateVelocity(Rigidbody *body, const float deltaTime);

        void Narrowphase(std::span<CollisionPair> inPair);

        void ResolveContacts(const CollisionPair& inPair, const class CollisionData* inCollisionData);

	private:
        bool      m_IsPaused{true};
        glm::vec3 m_Gravity;

        float m_AccumulateTime{0.f};

        uint32_t m_PositionIterations{2};

        uint32_t m_MaxBodiesCount{0};

        uint32_t m_RigidbodyCount{0};

        float m_DampingFactor{0.9995f};

        std::vector<Rigidbody *> m_Rigidbodies;
        std::vector<Rigidbody *> m_BodiesFreeList;

        std::unique_ptr<Broadphase> m_Broadphase;

        std::array<Manifold, MAX_MANIFOLDS> m_Manifolds;
        uint32_t                             m_ManifoldsCount{0};

        float s_UpdateTimestep;
	};
}
}
