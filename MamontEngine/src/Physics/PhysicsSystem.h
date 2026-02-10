#pragma once

#include <entt/entt.hpp>

namespace MamontEngine
{
    namespace HeroPhysics
    {
    class Rigidbody;
    class Broadphase;
    
    struct PhysicsSettings
    {
        float TimeStep{1.f / 120.f};
        glm::vec3 Gravity{glm::vec3(0.f, -9.81f, 0.f)};

        uint32_t MaxBodyCount{4096};
    };

	class PhysicsSystem
	{
    public:
        PhysicsSystem(const PhysicsSettings &settings = {});
        ~PhysicsSystem();

        void Update(const float inDeltaTime, entt::registry &inRegistry);

        inline bool IsPaused() const { return m_IsPaused; }

        inline void SetPause(bool inValue) { m_IsPaused = inValue; }

        Rigidbody *CreateBody();

        void DestroyBody(Rigidbody *body);

    private:
        void UpdateRigidbodies(entt::registry &inRegistry);

        void UpdateRigidbody(Rigidbody *body, const float deltaTime);

	private:
        bool      m_IsPaused{true};
        glm::vec3 m_Gravity;

        uint32_t m_PositionIterations{2};

        uint32_t m_MaxBodiesCount{0};

        uint32_t m_RigidbodyCount{0};

        float m_DampingFactor{0.9995f};

        std::vector<Rigidbody *> m_Rigidbodies;
        std::vector<Rigidbody *> m_BodiesFreeList;

        std::unique_ptr<Broadphase> m_Broadphase;
	};
}
}
