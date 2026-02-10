#include "Physics/PhysicsSystem.h"
#include "Core/Log.h"
#include "ECS/Components/RigidbodyComponent.h"
#include "ECS/Components/TransformComponent.h"
#include "ECS/Components/TagComponent.h"
#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/CollisionShape.h"
#include "Physics/Collision/Broadphase/BruteForceBroadphase.h"
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
} // namespace

namespace MamontEngine
{
    namespace HeroPhysics
    {
        static float s_UpdateTimestep = 1.f / 60.f;

        PhysicsSystem::PhysicsSystem(const PhysicsSettings &settings) : m_Gravity(settings.Gravity), m_MaxBodiesCount(settings.MaxBodyCount)
        {
            m_Rigidbodies.resize(m_MaxBodiesCount);
            m_BodiesFreeList.reserve(m_MaxBodiesCount);

            //m_Broadphase = std::make_unique<BruteForceBroadphase>();
        }

        PhysicsSystem::~PhysicsSystem()
        {
            m_Rigidbodies.clear();

            for (auto *&body : m_Rigidbodies)
            {
                delete body;
            }
        }

        void PhysicsSystem::Update(const float inDeltaTime, entt::registry &inRegistry)
        {
            PROFILE_FUNCTION();

            if (m_IsPaused) return;

            std::vector<CollisionPair> pairs;
            pairs.reserve(m_RigidbodyCount);

            for (uint32_t i = 0; i < m_RigidbodyCount; i++)
            {
                for (uint32_t j{ i }; j < m_RigidbodyCount; j++)
                {
                    const auto body1 = m_Rigidbodies[i];
                    const auto body2 = m_Rigidbodies[j];
                    if (!body1 || !body2) continue;

                    const auto &shape1 = body1->GetShape();
                    const auto &shape2 = body2->GetShape();
                    if (!shape1 || !shape2) continue;

                    const AABB object1 = shape1->GetBounds().Transform(ToMatrix4(body1->GetPosition(), body1->GetPosition()));
                    const AABB object2 = shape2->GetBounds().Transform(ToMatrix4(body2->GetPosition(), body2->GetRotation()));

                    if (object1.TestOverlap(object2))
                    {
                        pairs.push_back({body1, body2});
                    }
                }
            }

            //m_Broadphase->FindCollisionPairs(m_Rigidbodies., pairs, m_RigidbodyCount);

            UpdateRigidbodies(inRegistry);

            auto viewTransformsEnd = inRegistry.view<TransformComponent, RigidbodyComponent>();
            for (auto [entity, transfrom, rigidbody] : viewTransformsEnd.each())
            {
                transfrom.Transform.Position = rigidbody.Rigidbody->GetPosition();
                transfrom.Transform.Rotation = rigidbody.Rigidbody->GetRotation();
            }
        }

        void PhysicsSystem::UpdateRigidbodies(entt::registry &inRegistry)
        {
            PROFILE_FUNCTION();

            const auto viewBodies = inRegistry.view<RigidbodyComponent>();

            const float deltaTime = s_UpdateTimestep / float(m_PositionIterations);

            for (uint32_t i = 0; i < m_PositionIterations; i++)
            {
                uint32_t bodies = 0;
                for (uint32_t bodyIndex = 0; bodyIndex < m_RigidbodyCount; bodyIndex++)
                {
                    auto& body = m_Rigidbodies[bodyIndex];
                    UpdateRigidbody(body, deltaTime);
                    bodies++;
                }
            }
        }

        void PhysicsSystem::UpdateRigidbody(Rigidbody *body, const float deltaTime)
        {
            PROFILE_FUNCTION();

            if (!body)
            {
                Log::Warn("[PhysicsSystem] UpdateRigidbody: Body is null");
                return;
            }

            if (!body->IsStatic())
            {
                if (body->GetMass() > 0.f)
                {
                    const auto newLinearVelocity = body->GetLinearVelocity() + m_Gravity * deltaTime;
                    body->SetLinearVelocity(newLinearVelocity);

                    const glm::vec3 newPosition = body->GetPosition() + body->GetLinearVelocity() * deltaTime;
                    body->SetPosition(newPosition);

                    const glm::vec3 newAngularVelocity = body->GetAngularVelocity() + (body->GetTorque() * body->GetInverseInertia() * deltaTime);
                    body->SetAngularVelocity(newAngularVelocity * m_DampingFactor * body->GetAngularFactor());

                    const glm::vec3 angularVelocity = body->GetAngularVelocity() * deltaTime;

                    const glm::quat newRotation = body->GetRotation() + QuatMulVec3(body->GetRotation(), angularVelocity);
                    body->SetRotation(glm::normalize(newRotation));
                }
            }

        }

        Rigidbody *PhysicsSystem::CreateBody()
        {
            if (!m_BodiesFreeList.empty())
            {
                Rigidbody *body = m_BodiesFreeList.back();
                m_BodiesFreeList.pop_back();
                Log::Info("Create body from freelist");
                return body;
            }

            if (m_RigidbodyCount < m_MaxBodiesCount)
            {
                Rigidbody*& body = m_Rigidbodies[m_RigidbodyCount];
                body             = new Rigidbody();
                m_RigidbodyCount++;
                Log::Info("Create body: {}", (uint64_t)body->GetID());
                return body;
            }

            Log::Error("Physics System: Exceeded max rigidbody count {}", m_RigidbodyCount);
            return nullptr;
        }

        void PhysicsSystem::DestroyBody(Rigidbody *body)
        {
            if (body)
            {
                m_BodiesFreeList.push_back(body);
            }
        }

    } // namespace HeroPhysics
} // namespace MamontEngine
