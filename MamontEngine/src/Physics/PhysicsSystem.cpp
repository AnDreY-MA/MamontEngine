#include "Physics/PhysicsSystem.h"
#include "Core/Log.h"
#include "ECS/Components/RigidbodyComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/TagComponent.h"
#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/CollisionShape.h"
#include "Physics/Collision/Broadphase/BruteForceBroadphase.h"
#include "Physics/Collision/CollisionDetection.h"
#include "Utils/Profile.h"
#include "Math/Transform.h"

namespace
{
    glm::quat QuatMulVec3(const glm::quat &quat, const glm::vec3 vector)
    {
        glm::quat result{};

        result.w = -(quat.x * vector.x) - (quat.y * vector.y) - (quat.z * vector.z);

        result.x = (quat.w * vector.x) + (vector.y * quat.z) - (vector.z * quat.y);
        result.y = (quat.w * vector.y) + (vector.z * quat.x) - (vector.x * quat.z);
        result.z = (quat.w * vector.z) + (vector.x * quat.y) - (vector.y * quat.x);

        return result;
    }

    glm::mat4 ToMatrix4(const glm::vec3& inPosition, const glm::quat& inRotation)
    {
        glm::mat4 matrix{glm::mat4(1.0f)};

        matrix = glm::translate(matrix, inPosition);
        matrix = glm::rotate(matrix, glm::radians(inRotation.x), MamontEngine::RIGHT_VECTOR);
        matrix = glm::rotate(matrix, glm::radians(inRotation.y), MamontEngine::UP_VECTOR);
        matrix = glm::rotate(matrix, glm::radians(inRotation.z), MamontEngine::FORWARD_VECTOR);
        //matrix = glm::scale(matrix, Scale);

        return matrix;
    }

    static constexpr float M_EPSILON = 0.000001f;
} // namespace

namespace MamontEngine
{
    namespace HeroPhysics
    {
        PhysicsSystem::PhysicsSystem(const PhysicsSettings &settings) 
            : m_Gravity(settings.Gravity), m_MaxBodiesCount(settings.MaxBodyCount)
        {
            m_Rigidbodies.resize(m_MaxBodiesCount);
            //m_BodiesFreeList.reserve(m_MaxBodiesCount);

            m_Broadphase = std::make_unique<BruteForceBroadphase>();

            m_Manifolds.resize(1000);

            s_UpdateTimestep = 1.0f / 60.0f;
        }

        PhysicsSystem::~PhysicsSystem()
        {
            m_Rigidbodies.clear();

            for (auto *&body : m_Rigidbodies)
            {
                delete body;
            }
        }

        void PhysicsSystem::Update(const float inDeltaTime)
        {
            PROFILE_FUNCTION();

            if (m_IsPaused) return;

            m_AccumulateTime += inDeltaTime;
            for (uint32_t i = 0; (m_AccumulateTime + M_EPSILON >= s_UpdateTimestep) && i < 5; ++i)
            {
                m_AccumulateTime -= s_UpdateTimestep;

                std::vector<CollisionPair> pairs;
                //pairs.reserve(m_RigidbodyCount);

                m_Broadphase->FindCollisionPairs(m_Rigidbodies, pairs, m_RigidbodyCount);

                ResolveCollisions(pairs);

                //Log::Info("[PhysicsSystem] Collision pairs: {}", pairs.size());

                UpdateRigidbodies();

            }

            if (m_AccumulateTime + M_EPSILON >= s_UpdateTimestep)
            {
                m_AccumulateTime = std::fmod(m_AccumulateTime, s_UpdateTimestep);
            }

            /*   while (m_AccumulateTime >= s_UpdateTimestep)
            {
                std::vector<CollisionPair> pairs;
                pairs.reserve(m_RigidbodyCount);

                m_Broadphase->FindCollisionPairs(m_Rigidbodies, pairs, m_RigidbodyCount);

                UpdateRigidbodies();
                m_AccumulateTime -= s_UpdateTimestep;
            }            */
            
        }

        void PhysicsSystem::UpdateRigidbodies()
        {
            PROFILE_FUNCTION();

            Log::Info("Update bodies: {}", m_RigidbodyCount);

            const float deltaTime = s_UpdateTimestep;

            for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
            {
                auto &body = m_Rigidbodies[bodyIndex];
                UpdateRigidbody(body, deltaTime);
            }

             /*for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
            {
                auto &body = m_Rigidbodies[bodyIndex];
                if (!body->IsStatic())
                {
                    body->m_Force += m_Gravity * body->m_Mass;
                    
                }
                Log::Info("Focrce");
            }

            for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
            {
                auto &body = m_Rigidbodies[bodyIndex];
                if (!body->IsStatic())
                {
                    IntegrateForce(body, deltaTime);
                }
            }

            for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
            {
                auto &body = m_Rigidbodies[bodyIndex];
                if (!body->IsStatic())
                {
                    IntegrateVelocity(body, deltaTime);
                }
            }

            for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
            {
                auto &body = m_Rigidbodies[bodyIndex];
                body->m_Force = glm::vec3(0.f);
                body->m_Torque = glm::vec3(0.f);
            }*/
        }

        void PhysicsSystem::UpdateRigidbody(Rigidbody *body, const float deltaTime)
        {
            PROFILE_FUNCTION();

            if (!body)
            {
                Log::Warn("[PhysicsSystem] UpdateRigidbody: Body is null");
                return;
            }

            s_UpdateTimestep /= m_PositionIterations;

            if (!body->IsStatic())
            {
                body->m_LinearVelocity += (m_Gravity * body->m_GravityScale) * s_UpdateTimestep;
                body->m_LinearVelocity = body->m_LinearVelocity * m_DampingFactor;

                body->m_Position += body->GetLinearVelocity() * s_UpdateTimestep;

                const glm::vec3 newAngularVelocity = body->GetAngularVelocity() + (body->GetTorque() * body->GetInverseInertia() * s_UpdateTimestep);
                body->SetAngularVelocity(newAngularVelocity * m_DampingFactor * body->GetAngularFactor());

                const glm::vec3 angularVelocity = body->GetAngularVelocity() * s_UpdateTimestep;

                const glm::quat newRotation = body->GetRotation() + QuatMulVec3(body->GetRotation(), angularVelocity);
                body->SetRotation(glm::normalize(newRotation));
            }

            s_UpdateTimestep *= m_PositionIterations;

        }

        void PhysicsSystem::IntegrateForce(Rigidbody* body, const float deltaTime)
        {
            body->m_LinearVelocity += (body->m_Force * body->GetInverseMass()) * deltaTime;

            body->m_AngularVelocity += glm::vec3(glm::mat3(1.0f)/*InverseInertiaTensor*/ * glm::vec4(body->m_Torque, 0.0f)) * deltaTime;

            const float linierDamping{0.01f};
            const float angularDamping{0.01f}; 

            body->m_LinearVelocity *= (1.0f - linierDamping);
            body->m_AngularVelocity *= (1.0f - angularDamping);
        }

        void PhysicsSystem::IntegrateVelocity(Rigidbody *body, const float deltaTime)
        {
            body->m_Position += body->m_LinearVelocity * deltaTime;

            const glm::quat angularVelocityQuat(0.0f, body->m_AngularVelocity.x, body->m_AngularVelocity.y, body->m_AngularVelocity.z);
            body->m_Rotation += (angularVelocityQuat * body->m_Rotation) * 0.5f * deltaTime;
            body->m_Rotation = glm::normalize(body->m_Rotation);
        }

        void PhysicsSystem::ResolveCollisions(std::span<CollisionPair> inPair)
        {
            for (auto& pair : inPair)
            {
                auto body1 = pair.Object1;
                auto body2 = pair.Object2;

                if (!body1 || !body2)
                    continue;

                CollisionData collisionData{};
                if (!CheckCollision(&pair, &collisionData))
                {
                    Log::Info("Check collision false");
                    continue;
                }

                const glm::vec3 relativeVelocity = body2->GetLinearVelocity() - body1->GetLinearVelocity();

                const float velocityAlongNormal{glm::dot(relativeVelocity, collisionData.Normal)};
                if (velocityAlongNormal > 0)
                    continue;

                const float restitution = std::min(body1->GetRestitution(), body2->GetRestitution());

                float scalarImpulse = -(1.0f + restitution) * velocityAlongNormal;
                scalarImpulse /= body1->GetInverseMass() + body2->GetInverseMass();

                const glm::vec3 impulse = collisionData.Normal * scalarImpulse;

                if (!body1->IsKinematic() || !body1->IsStatic())
                {
                    body1->m_LinearVelocity -= impulse * body1->GetInverseMass();
                }
                if (!body2->IsKinematic() || !body2->IsStatic())
                {
                    body2->m_LinearVelocity += impulse * body2->GetInverseMass();
                }

                const float percent{0.2f};
                const float slop = 0.01f;

                const glm::vec3 correction = std::max(collisionData.Penetration - slop, 0.0f) * percent * collisionData.Normal / (body1->GetInverseMass() + body2->GetInverseMass());

                if (!body1->IsKinematic() || !body1->IsStatic())
                {
                    body1->m_Position -= correction * body1->GetInverseMass();
                }
                if (!body2->IsKinematic() || !body2->IsStatic())
                {
                    body2->m_Position += correction * body2->GetInverseMass();
                }
            }
        }

        void PhysicsSystem::NarrowPhaseCollisions(std::span<CollisionPair> inPair)
        {
            for (auto &pair : inPair)
            {
                auto body1 = pair.Object1;
                auto body2 = pair.Object2;

                if (!body1 || !body2)
                    continue;

                CollisionData collisionData{};

                if (CheckCollision(&pair, &collisionData))
                {
                    Manifold &manifold = m_Manifolds[m_ManifoldCount++];
                    manifold.Init(body1, body2);


                }
            }
        }

        Rigidbody *PhysicsSystem::CreateBody()
        {
            if (m_BodiesFreeList.size() > 0)
            {
                Rigidbody *body = m_BodiesFreeList.back();
                m_BodiesFreeList.pop_back();
                Log::Info("Create body from freelist");
                return body;
            }

            if (m_RigidbodyCount < m_MaxBodiesCount)
            {
                Rigidbody *&body = m_Rigidbodies[m_RigidbodyCount];
                body             = new Rigidbody();
                ++m_RigidbodyCount;
                Log::Info("Create body: {}", (uint64_t)body->GetID());
                return body;
            }

            return nullptr;
        }

        void PhysicsSystem::DestroyBody(Rigidbody *body)
        {
            if (body)
            {
                Log::Info("Destroy Body");
                m_BodiesFreeList.push_back(body);
            }
        }

    } // namespace HeroPhysics
} // namespace MamontEngine
