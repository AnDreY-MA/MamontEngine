#pragma once

#include "Core/UID.h"
#include "Utils/MetaReflection.h"

namespace MamontEngine
{
namespace HeroPhysics
{
    enum class EMotionType : uint8_t{ Static, Dynamic, Kinematic };

    class CollisionShape;

	class Rigidbody final : public NonCopyable
    {
       // REFLECT()

    public:
        Rigidbody();
        ~Rigidbody() = default;

        inline UID GetID() const
        {
            return m_Id;
        }

        inline const glm::vec3 &GetPosition() const
        {
            return m_Position;
        }
        inline const glm::quat &GetRotation() const
        {
            return m_Rotation;
        }

        inline const glm::vec3 &GetLinearVelocity() const
        {
            return m_LinearVelocity;
        }
        inline const glm::vec3 &GetAngularVelocity() const
        {
            return m_AngularVelocity;
        }

        inline const glm::vec3 &GetTorque() const
        {
            return m_Torque;
        }

        inline const glm::mat3 &GetInverseInertia() const { return m_InverseInertia; }

        inline float GetMass() const { return m_Mass; }

        inline float GetAngularFactor() const { return m_AngularFactor; }

        inline bool IsStatic() const { return m_MotionType == EMotionType::Static; }

        const std::shared_ptr<const CollisionShape> &GetShape() const { return m_Shape; }

        void SetCollisionShape(const std::shared_ptr<CollisionShape> &inShape);

        inline void SetPosition(const glm::vec3 &inValue) { m_Position = inValue; }

        inline void SetRotation(const glm::quat &inValue) { m_Rotation = inValue; }

        inline void SetLinearVelocity(const glm::vec3 &inValue) { m_LinearVelocity = inValue; }

        inline void SetAngularVelocity(const glm::vec3 &inValue) { m_AngularVelocity = inValue; }

        inline void SetMass(float inMass) { m_Mass = inMass; }

    private:
        glm::vec3                             m_Position{glm::vec3(0.f)};
        glm::quat                             m_Rotation;
        std::shared_ptr<CollisionShape> m_Shape;

        EMotionType m_MotionType{EMotionType::Dynamic};

        glm::vec3 m_LinearVelocity{glm::vec3(0.f)};
        glm::vec3 m_AngularVelocity{glm::vec3(0.f)};
        glm::vec3 m_Force{glm::vec3(0.f)};
        float     m_Mass{1.f};

        float m_Friction{0.5f};

        float m_AngularFactor{1.f};

        UID m_Id;

		glm::vec3 m_Gravity;

        glm::vec3 m_Torque{glm::vec3(0.f)};

        glm::mat3 m_InverseInertia{glm::mat3(1.f)};
	};
}
}