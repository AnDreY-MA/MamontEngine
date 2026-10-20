#include "Physics/Collision/CollisionDetection.h"
#include "Utils/Profile.h"
#include "Physics/Collision/Broadphase/Broadphase.h"
#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/BoxCollision.h"
#include "Physics/Collision/SphereCollision.h"
#include <glm/gtx/norm.hpp>
#include "Core/Log.h"

namespace
{
    std::array<glm::vec3, 3> GetCollisionAxes(const glm::quat& inOrientation)
    {
        std::array<glm::vec3, 3> axes{};

        glm::mat3 objectOrientation = glm::mat3(inOrientation);
        axes[0]                     = (objectOrientation * glm::vec3(1.0f, 0.f, 0.f));
        axes[1]                     = (objectOrientation * glm::vec3(0.0f, 1.f, 0.f));
        axes[2]                     = (objectOrientation * glm::vec3(0.0f, 0.f, 1.f));
        
        return axes;
    }
}

namespace MamontEngine
{
    namespace HeroPhysics
    {
        bool CheckCollisionAxis(
                const glm::vec3 inAxis, Rigidbody *body1, Rigidbody *body2, CollisionShape *shape1, CollisionShape *shape2, CollisionData *outCollisionData);

        static bool CheckCollisionBoxBox(const CollisionPair *inPair, CollisionData* outCollisionData)
        {
            CollisionData currentData;
            CollisionData bestData; 
            bestData.Penetration = -FLT_MAX;

            const auto& shape1 = inPair->Object1->GetShape();
            const auto& shape2 = inPair->Object2->GetShape();

            const std::vector<glm::vec3> &shape1CollisionAxes = shape1->GetAxes(inPair->Object1->GetRotation());
            const std::vector<glm::vec3> &shape2CollisionAxes = shape2->GetAxes(inPair->Object2->GetRotation());

            static constexpr int MAX_COLLISION_AXES = 100;
            static std::array<glm::vec3, MAX_COLLISION_AXES> possibleCollisionAxes{};

            uint32_t possibleCollisionAxesCount{0};
            for (const glm::vec3 &axis : shape1CollisionAxes)
            {
                possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
            }

            for (const glm::vec3 &axis : shape2CollisionAxes)
            {
                possibleCollisionAxes[possibleCollisionAxesCount++] = axis;
            }

            for (uint32_t i = 0; i < possibleCollisionAxesCount; ++i)
            {
                const glm::vec3 &axis = possibleCollisionAxes[i];

                if (!CheckCollisionAxis(axis, inPair->Object1, inPair->Object2, shape1.get(), shape2.get(), &currentData))
                    return false;

                if (currentData.Penetration >= bestData.Penetration)
                    bestData = currentData;
            }

            if (outCollisionData)
            {
                *outCollisionData = bestData;
            }

            return true;
        }

        static bool CheckSpherSphere(const CollisionPair* inPair, CollisionData* collisionData)
        {
            auto body1   = inPair->Object1;
            auto body2   = inPair->Object2;

            if (!body1 || !body2)
                return false;

            auto sphere1 = std::static_pointer_cast<SphereCollision>(body1->GetShape());
            auto sphere2 = std::static_pointer_cast<SphereCollision>(body2->GetShape());

           /* glm::vec3 direction{body2->GetPosition() - body1->GetPosition()};
            const float     distance = glm::length(direction);
            const float     minDistance{sphere1->GetRadius() + sphere2->GetRadius()};

            if (distance >= minDistance)
            {
                Log::Info("CheckSpherSphere distance false");
                return false;
            }
            direction = distance > 0.0001f ? direction / distance : glm::vec3(0.f, 1.f, 0.f);

            collisionData->Point = body1->GetPosition() + direction * sphere1->GetRadius();
            collisionData->Normal = direction;
            collisionData->Penetration = minDistance - distance;*/

            glm::vec3 axis = body2->GetPosition() - body1->GetPosition();
            const float sumRadii{sphere1->GetRadius() + sphere2->GetRadius()};
            const float sumRadiiSquared = sumRadii * sumRadii;
            const float distSquared     = glm::length2(axis);
            if (distSquared > sumRadiiSquared)
                return false;

            collisionData->Normal = glm::normalize(axis);
            collisionData->Penetration = sumRadii - glm::sqrt(distSquared);
            collisionData->Point       = body1->GetPosition() + axis * 0.5f;

            return true;
        }

        bool CheckCollision(const CollisionPair *inPair, CollisionData *collisionData)
        {
            const auto &shape1 = inPair->Object1->GetShape();
            const auto &shape2 = inPair->Object2->GetShape();

            if (!shape1 || !shape2)
            {
                Log::Warn("[CheckCollision] Shapes is not valid!");
                return false;
            }

            switch (shape1->GetShapeType())
            {
                //case EShapeType::Box:
                    
                default:
                    break;
            }

            /* if (shape1->GetShapeType() == EShapeType::Sphere)
            {
                if (shape2->GetShapeType() == EShapeType::Sphere)
                {
                    return CheckSpherSphere(inPair, collisionData);
                }
            }*/

            return CheckSpherSphere(inPair, collisionData);
        }

        bool BuildCollisionManifold(Rigidbody *body1, Rigidbody *body2, CollisionData &collisionData, Manifold *manifold)
        {
            if (!manifold) return false;

            const auto& shape1 = body1->GetShape();
            const auto& shape2 = body2->GetShape();

            ReferencePolygon poly1, poly2;
            shape1->GetIncindentRefPolygon(body1->GetWorldTransform(), collisionData.Normal, poly1);
            shape2->GetIncindentRefPolygon(body2->GetWorldTransform(), -collisionData.Normal, poly2);

            // other not writen yet code

            return false;
        }

        bool CheckCollisionAxis(
            const glm::vec3 inAxis, Rigidbody* body1, Rigidbody* body2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* outCollisionData)
        {
            const auto obj1 = shape1->GetMinMaxVertexOnAxis(body1->GetWorldTransform(), inAxis);
            const auto obj2 = shape2->GetMinMaxVertexOnAxis(body2->GetWorldTransform(), inAxis);

            const float minCorrelation1 = glm::dot(inAxis, obj1.first);
            const float maxCorrelation1 = glm::dot(inAxis, obj1.second);
            const float minCorrelation2 = glm::dot(inAxis, obj2.first);
            const float maxCorrelation2 = glm::dot(inAxis, obj2.second);

            if (minCorrelation1 <= minCorrelation2 && maxCorrelation1 >= maxCorrelation2)
            {
                outCollisionData->Normal = inAxis;
                outCollisionData->Penetration = minCorrelation2 - maxCorrelation1;
                outCollisionData->Point       = obj1.second + inAxis * outCollisionData->Penetration;
                return true;
            }

            if (minCorrelation2 <= minCorrelation1 && maxCorrelation2 >= maxCorrelation1)
            {
                outCollisionData->Normal      = -inAxis;
                outCollisionData->Penetration = minCorrelation1 - maxCorrelation2;
                outCollisionData->Point       = obj1.first + inAxis * outCollisionData->Penetration;
                return true;    
            }

            return false;
        }
    } // namespace HeroPhysics
} // namespace MamontEngine