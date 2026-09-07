#pragma once 

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class Rigidbody;

        #define MAX_CONTACT_POINTS 8

        struct ContactPoint
        {
            float Penetration{0.f};

            glm::vec3 Normal;
            glm::vec3 RelativePos1;
            glm::vec3 RelativePos2;
        };

        class Manifold
        {
        public:
            explicit Manifold();
            ~Manifold();

            void Init(Rigidbody *body1, Rigidbody* body2)
            {
                m_Body1 = body1;
                m_Body2 = body2;
            }

        private:
            Rigidbody *m_Body1;
            Rigidbody *m_Body2;

            std::array<ContactPoint, MAX_CONTACT_POINTS> m_Contacts;
            uint32_t                                     m_ContactCount{0};

            float m_BaumgarteScalar{0.2f};
            float m_BaumgarteSlop{0.001f};
        };
    }
} // namespace MamontEngine
