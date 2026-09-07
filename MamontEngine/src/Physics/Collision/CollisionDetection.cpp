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
        static std::vector<glm::vec3> GetBoxVertices(const AABB &aabb)
        {
            std::vector<glm::vec3> vertices;
            vertices.resize(8);
            
            // Lower
            vertices[0].x = aabb.Min.x;
            vertices[0].y = aabb.Min.y;
            vertices[0].z = aabb.Min.z;

            vertices[1].x = aabb.Min.x;
            vertices[1].z = aabb.Min.z;

            vertices[2].z = aabb.Min.z;

            vertices[3].y = aabb.Min.y;
            vertices[3].z = aabb.Min.z;

            vertices[4].x = aabb.Min.x;
            vertices[4].y = aabb.Min.y;

            vertices[5].x = aabb.Min.x;

            vertices[7].y = aabb.Min.y;

            // Upper
            vertices[1].y = aabb.Max.y;

            vertices[2].x = aabb.Max.x;
            vertices[2].y = aabb.Max.y;

            vertices[3].x = aabb.Max.x;

            vertices[4].z = aabb.Max.z;

            vertices[5].y = aabb.Max.y;
            vertices[5].z = aabb.Max.z;

            vertices[6].x = aabb.Max.x;
            vertices[6].y = aabb.Max.y;
            vertices[6].z = aabb.Max.z;

            vertices[7].x = aabb.Max.x;
            vertices[7].z = aabb.Max.z;

            return vertices;
        }

        static std::pair<glm::vec3, glm::vec3> GetMinMaxVertexOnAxis(const Rigidbody* body, const glm::vec3& axis)
        {
            const glm::mat4 currentTransform = body->GetWorldTransform();

            const glm::vec3 localAxis = glm::transpose(glm::mat3(currentTransform)) * axis;

            const auto vertices = GetBoxVertices(body->GetShape()->GetBounds());

            int minVertex = 0, maxVertex = 0;
            {
                float minCorrelation = FLT_MAX, maxCorrelation = -FLT_MAX;
                for (size_t i = 0; i < vertices.size(); ++i)
                {
                    const float currentCorrelation = glm::dot(localAxis, vertices[i]);
                    if (currentCorrelation > maxCorrelation)
                    {
                        maxCorrelation = currentCorrelation;
                        maxVertex      = int(i);
                    }
                    if (currentCorrelation <= minCorrelation)
                    {
                        minCorrelation = currentCorrelation;
                        minVertex      = int(i);
                    }
                }
            }

            std::pair<glm::vec3, glm::vec3> result;

            result.first = currentTransform * glm::vec4(vertices[minVertex], 1.f);
            result.second = currentTransform * glm::vec4(vertices[maxVertex], 1.f);

            return result;
        }

        static void GetIncidentReferencePolygon(const Rigidbody* body, const glm::vec3& axis, ReferencePolygon& refPolygon)
        {
            const glm::mat4 currentTransform = body->GetWorldTransform();

            const glm::mat3 invNormalMatrix{glm::transpose(glm::mat3(currentTransform))};
            const glm::mat3 normalMatrix{glm::inverse(invNormalMatrix)};

            const glm::vec3 localAxis = invNormalMatrix * axis;
            
            const auto minmaxVertices = GetMinMaxVertexOnAxis(body, localAxis);

            const auto boxVertices = GetBoxVertices(body->GetShape()->GetBounds());

            //const; 

        }

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

            ReferencePolygon poly1, poly2;


            return false;
        }

        bool CheckCollisionAxis(
            const glm::vec3 inAxis, Rigidbody* body1, Rigidbody* body2, CollisionShape* shape1, CollisionShape* shape2, CollisionData* outCollisionData)
        {
            const auto obj1 = GetMinMaxVertexOnAxis(body1, inAxis);
            const auto obj2 = GetMinMaxVertexOnAxis(body2, inAxis);

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