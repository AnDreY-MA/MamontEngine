#pragma once

#include "Physics/Collision/CollisionShape.h"
#include "Utils/MetaReflection.h"
#include <cereal/cereal.hpp>

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class SphereCollision : public CollisionShape
        {
            REFLECT()
        public:
            explicit SphereCollision() : CollisionShape(EShapeType::Sphere)
            {
            }

            explicit SphereCollision(float inRadius) : CollisionShape(EShapeType::Box), m_Radius(inRadius)
            {
            }

            inline float GetRadius() const
            {
                return m_Radius;
            }

            virtual AABB GetBounds() const override
            {
                const glm::vec3 halfExtent = glm::vec3(m_Radius, m_Radius, m_Radius);
                return AABB(-halfExtent, halfExtent);
            }

        private:
            float m_Radius{1.f};

            friend class cereal::access;
            template <typename Archive>
            void serialize(Archive &ar)
            {
                ar(m_Radius);
            }
        };
    } // namespace HeroPhysics
} // namespace MamontEngine
