#pragma once

#include "Physics/Collision/CollisionShape.h"
#include "Utils/MetaReflection.h"
#include <cereal/cereal.hpp>

namespace MamontEngine
{
    namespace HeroPhysics
    {
    class BoxCollision : public CollisionShape
	{
        REFLECT()
    public:
        BoxCollision() 
            : CollisionShape(EShapeType::Box)
        {
        }

        explicit BoxCollision(const glm::vec3& inExtent) : CollisionShape(EShapeType::Box)
            , m_HalfExtent(inExtent)
        {

        }

        const glm::vec3 &GetHalfExtent() const
        {
            return m_HalfExtent;
        }

        virtual AABB GetBounds() const override
        {
            return AABB(-m_HalfExtent, m_HalfExtent);
        }

        virtual glm::mat3 BuildInverseInertia(const float inMass) const override;

        virtual std::vector<glm::vec3> GetAxes(const glm::quat &inOrientation) const override;

        virtual std::vector<CollisionEdge> GetEdges(const glm::mat4 &inTransform) const override;

	private:
        
        glm::vec3 m_HalfExtent{glm::vec3(0.f)};
        
        friend class cereal::access;
        template <typename Archive>
        void serialize(Archive &ar)
        {
            ar(m_HalfExtent);
        }
	};
    }
}