#pragma once

#include "Math/AABB.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        enum class EShapeType : uint8_t
        {
            Box, Sphere, Capsule, Cylinder, Empty
        };

        class CollisionShape
        {
        public:
            CollisionShape() = default;

            explicit CollisionShape(EShapeType inType = EShapeType::Empty)
                : m_ShapeType(inType)
            {

            }

            virtual ~CollisionShape() = default;


            virtual AABB GetBounds() const = 0;

            virtual glm::mat3 BuildInverseInertia(const float inMass) const = 0;
            
            EShapeType GetShapeType() const { return m_ShapeType; }

            bool IsTrigger() const { return m_IsTrigger; }

        private:
            EShapeType m_ShapeType{EShapeType::Empty};

            bool m_IsTrigger{false};
        };
    }
} // namespace MamontEngine
