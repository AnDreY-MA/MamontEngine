#include "Physics/Collision/Broadphase/BruteForceBroadphase.h"
#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/BoxCollision.h"
#include "Math/Transform.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Utils/Profile.h"

namespace MamontEngine
{
    namespace HeroPhysics
    {
        static glm::mat4 ToMatrix4(const glm::vec3 &inPosition, const glm::quat &inRotation)
        {
            glm::mat4 matrix{glm::mat4(1.0f)};

            matrix = glm::translate(matrix, inPosition);
            matrix = glm::rotate(matrix, glm::radians(inRotation.x), MamontEngine::RIGHT_VECTOR);
            matrix = glm::rotate(matrix, glm::radians(inRotation.y), MamontEngine::UP_VECTOR);
            matrix = glm::rotate(matrix, glm::radians(inRotation.z), MamontEngine::FORWARD_VECTOR);
            // matrix = glm::scale(matrix, Scale);

            return matrix;
        }

        void BruteForceBroadphase::FindCollisionPairs(const std::vector<Rigidbody*> &inBodies, std::vector<CollisionPair> &inCollisionPairs, uint32_t totalBodyCount)
        {
            PROFILE_FUNCTION();

            for (uint32_t i = 0; i < totalBodyCount; i++)
            {
                for (uint32_t j{i + 1}; j < totalBodyCount; j++)
                {
                    const auto body1 = inBodies[i];
                    const auto body2 = inBodies[j];
                    if (!body1 || !body2)
                        continue;

                    const auto &shape1 = body1->GetShape();
                    const auto &shape2 = body2->GetShape();

                    if (!shape1 || !shape2)
                        continue;

                    const AABB object1 = shape1->GetBounds().Transform(ToMatrix4(body1->GetPosition(), body1->GetRotation()));
                    const AABB object2 = shape2->GetBounds().Transform(ToMatrix4(body2->GetPosition(), body2->GetRotation()));

                    if (object1.TestOverlap(object2))
                    {
                        inCollisionPairs.push_back({body1, body2});
                    }
                }
            }
        }
    } // namespace HeroPhysics
} // namespace MamontEngine
