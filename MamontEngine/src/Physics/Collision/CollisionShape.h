#pragma once

#include "Math/AABB.h"
#include "Math/Plane.h"
#include "ECS/Components/Component.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class Rigidbody;

        enum class EShapeType : uint8_t
        {
            Box, Sphere, Capsule, Cylinder, Empty
        };

        struct ReferencePolygon
        {
            glm::vec3 Faces[8];
            Plane     AdjacentPlanes[8];

            glm::vec3 Normal;

            uint32_t FaceCount{0};
            uint32_t PlaneCount{0};
        };

        struct CollisionEdge
        {
            glm::vec3 PosA{glm::vec3(0.f)};
            glm::vec3 PosB{glm::vec3(0.f)};
        };

        class CollisionShape : public Component
        {
           // REFLECT();
        public:
            CollisionShape() = default;

            explicit CollisionShape(EShapeType inType = EShapeType::Empty)
                : m_ShapeType(inType)
            {

            }

            virtual ~CollisionShape() = default;

            virtual AABB GetBounds() const = 0;

            virtual std::vector<glm::vec3> GetAxes(const glm::quat& inOrientation) const = 0;

            virtual std::vector<CollisionEdge> GetEdges(const glm::mat4& inTransform) const = 0;

            virtual std::pair<int, int> GetMinMaxVertexInAxis(const glm::vec3 &axis) const
            {
                return {0, 0};
            };

            virtual std::pair<glm::vec3, glm::vec3> GetMinMaxVertexOnAxis(const glm::mat4 &inWorldTransform, const glm::vec3 &axis) const
            {
                return {};
            };

            
            virtual void GetIncindentRefPolygon(const glm::mat4 &inWorldTransform, const glm::vec3 &axis, ReferencePolygon &outRefPolygon) const
            {

            }

            virtual glm::mat3 BuildInverseInertia(const float inMass) const
            {
                return glm::mat3(1.f);
            }
            
            EShapeType GetShapeType() const { return m_ShapeType; }

            bool IsTrigger() const { return m_IsTrigger; }

        private:
            EShapeType m_ShapeType{EShapeType::Empty};

            bool m_IsTrigger{false};
        };
    }
} // namespace MamontEngine
