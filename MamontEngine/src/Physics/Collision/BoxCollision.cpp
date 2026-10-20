#include "Physics/Collision/BoxCollision.h"
#include "Utils/Reflection.h"
#include "Graphics/DebugRenderer.h"
#include "Core/Log.h"

namespace
{
    std::vector<glm::vec3> GetBoxVertices(const MamontEngine::AABB &aabb)
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
}

namespace MamontEngine
{
	namespace HeroPhysics
	{
        IMPLEMENT_REFLECT_COMPONENT(BoxCollision, "BoxCollision Component")
        {
            meta.data<&BoxCollision::m_HalfExtent>(entt::hashed_string{"HalfExtent"}, "Half Extent");
		}
        FINISH_REFLECT()

		glm::mat3 BoxCollision::BuildInverseInertia(const float inMass) const
        {
            glm::mat3 inertia{glm::mat3(1.f)};
            
            return inertia;
        }

        std::vector<glm::vec3> BoxCollision::GetAxes(const glm::quat &inOrientation) const
        {
            std::vector<glm::vec3> axes;
            axes.resize(3);

            const glm::mat3 matOrientation = glm::mat3(inOrientation);
            axes[0]                          = (matOrientation * glm::vec3(1.0f, 0.0f, 0.0f));
            axes[1]                  = (matOrientation * glm::vec3(0.0f, 1.0f, 0.0f));
            axes[2]                          = (matOrientation * glm::vec3(0.0f, 0.0f, 1.0f));

            return axes;
        }

        std::vector<CollisionEdge> BoxCollision::GetEdges(const glm::mat4 &inTransform) const
        {
            std::vector<CollisionEdge> edges;

            return edges;
        }

        std::pair<int, int> BoxCollision::GetMinMaxVertexInAxis(const glm::vec3 &axis) const
        {
            const auto vertices = GetBoxVertices(GetBounds());
            int minVertex = 0, maxVertex = 0;
            float      minCorrelation = FLT_MAX, maxCorrelation = -FLT_MAX;
            for (size_t i = 0; i < vertices.size(); ++i)
            {
                const float currentCorrelation = glm::dot(axis, vertices[i]);
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
            return {minVertex, maxVertex};
        }

        std::pair<glm::vec3, glm::vec3> BoxCollision::GetMinMaxVertexOnAxis(const glm::mat4 &inWorldTransform, const glm::vec3 &axis) const
        {
            const glm::vec3 localAxis = glm::transpose(glm::mat3(inWorldTransform)) * axis;

            const auto vertices = GetBoxVertices(GetBounds());

            const auto [minVertex, maxVertex] = GetMinMaxVertexInAxis(localAxis);

            std::pair<glm::vec3, glm::vec3> result;

            result.first  = inWorldTransform * glm::vec4(vertices[minVertex], 1.f);
            result.second = inWorldTransform * glm::vec4(vertices[maxVertex], 1.f);

            return result;
        }

        struct Line
        {
            glm::vec3 Start;
            glm::vec3 End;
        };

        void BoxCollision::GetIncindentRefPolygon(const glm::mat4 &inWorldTransform, const glm::vec3 &axis, ReferencePolygon &outRefPolygon) const
        {
            const glm::mat3 invNormalMatrix{glm::transpose(inWorldTransform)};
            const glm::mat3 normalMatrix{glm::inverse(invNormalMatrix)};

            const glm::vec3 localAxis{invNormalMatrix * axis};

            const auto [minVertex, maxVertex] = GetMinMaxVertexInAxis(localAxis);

            constexpr int indicesEdges[12][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}
            };
            const auto vertices = GetBoxVertices(GetBounds());

            std::vector<Line> edges;
            edges.reserve(12);

            for (size_t i = 0; i < 12; ++i)
            {
                const Line line{
                    .Start = vertices[indicesEdges[i][0]], .End = vertices[indicesEdges[i][1]]
                };
                edges.push_back(line);
            }

            for (const auto& edge : edges)
            {
                DebugRenderer::DrawLine(edge.Start, edge.End, Color::RED);
            }
        }
	}
}