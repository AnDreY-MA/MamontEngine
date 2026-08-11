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

    std::pair<glm::vec3, glm::vec3>
    closestPointsBetweenSegments(glm::vec3 lineOrig0, glm::vec3 dir0, float min0, float max0, glm::vec3 lineOrig1, glm::vec3 dir1, float min1, float max1)
    {

        glm::vec3 p0, p1;

        float     v1v2 = glm::dot(dir0, dir1);
        glm::vec3 r    = lineOrig1 - lineOrig0;
        float     rv1  = glm::dot(r, dir0);
        float     rv2  = glm::dot(r, dir1);

        float t1 = glm::abs(v1v2) > 1.f - 0.0001f ? 0 : (rv1 * v1v2 - rv2) / (1.f - v1v2 * v1v2);
        float t0 = rv1 + t1 * v1v2;

        if (t0 < min0)
        {
            t0 = min0;
            p0 = lineOrig0 + t0 * dir0;
            t1 = -glm::dot(lineOrig1 - p0, dir1);
        }
        else if (t0 > max0)
        {
            t0 = max0;
            p0 = lineOrig0 + t0 * dir0;
            t1 = -glm::dot(lineOrig1 - p0, dir1);
        }
        else
        {
            p0 = lineOrig0 + t0 * dir0;
        }

        if (t1 < min1)
        {
            t1 = min1;
            p1 = lineOrig1 + t1 * dir1;
            t0 = -glm::dot(lineOrig0 - p1, dir0);
            t0 = glm::clamp(t0, min0, max0);
            p0 = lineOrig0 + t0 * dir0;
        }
        else if (t1 > max1)
        {
            t1 = max1;
            p1 = lineOrig1 + t1 * dir1;
            t0 = -glm::dot(lineOrig0 - p1, dir0);
            t0 = glm::clamp(t0, min0, max0);
            p0 = lineOrig0 + t0 * dir0;
        }
        else
        {
            p1 = lineOrig1 + t1 * dir1;
        }

        return {p0, p1};
    }

    struct BoxEdgeContactInfo
    {
        int edge0{0};
        int edge1{0};
    };

    struct BoxFaceContactInfo
    {
        int Box{0};
        int Axis{0};
    };

    enum class EBoxContactType : uint8_t
    {
        FACE, EDGE
    };

    thread_local std::vector<glm::vec2> polygon;
    thread_local std::vector<glm::vec2> clip;
    constexpr float                     epsilon = 1e-4f;
    thread_local std::vector<glm::vec3> penetratedPoints;
    void generateContactsPolygonBoxFace(glm::vec3               boxCenter,
                                                 glm::mat3               boxBasis,
                                                 int                     boxAxis,
                                                 int                     boxAxisSign,
                                                 glm::vec3               boxHalfExtents,
                                                 glm::vec3               incPlaneOrig,
                                                 glm::vec3               incPlaneNormal,
                                                 std::vector<glm::vec2> &polygon,
                                                 int                     clipX,
                                                 int                     clipY,
                                                 MamontEngine::HeroPhysics::ContactPoint           *contactPoints,
                                                 int                    &numPoints)
    {
        penetratedPoints.clear();
        penetratedPoints.reserve(polygon.size());

        for (auto point : polygon)
        {
            glm::vec3 point3(0);
            point3[clipX] = point.x;
            point3[clipY] = point.y;
            glm::vec3 dir(0);
            dir[boxAxis]   = boxAxisSign;
            float distance = glm::dot((incPlaneOrig - point3), incPlaneNormal) / glm::dot(dir, incPlaneNormal); // intersection of ray and plane
            if (distance < boxHalfExtents[boxAxis] + epsilon)
            {
                point3[boxAxis] = boxAxisSign * distance;
                penetratedPoints.push_back(point3);
            }
        }

        if (penetratedPoints.size() > 4)
        {
            // build manifold from 4 points

            glm::vec3 penetratedPoint0 = penetratedPoints[0];
            penetratedPoint0[boxAxis]  = boxAxisSign * boxHalfExtents[boxAxis];
            contactPoints[0]           = {boxCenter + boxBasis * penetratedPoint0, boxCenter + boxBasis * penetratedPoints[0]};

            float maxDistance      = 0;
            int   maxDistanceIndex = 0;
            for (int i = 0; i < penetratedPoints.size(); ++i)
            {
                float distance = glm::distance2(penetratedPoints[0], penetratedPoints[i]);
                if (distance > maxDistance)
                {
                    maxDistance      = distance;
                    maxDistanceIndex = i;
                }
            }

            penetratedPoint0          = penetratedPoints[maxDistanceIndex];
            penetratedPoint0[boxAxis] = boxAxisSign * boxHalfExtents[boxAxis];
            contactPoints[1]          = {boxCenter + boxBasis * penetratedPoint0, boxCenter + boxBasis * penetratedPoints[maxDistanceIndex]};

            float maxArea      = 0;
            int   maxAreaIndex = 0;
            for (int i = 0; i < penetratedPoints.size(); ++i)
            {
                auto  ca   = penetratedPoints[0] - penetratedPoints[i];
                auto  cb   = penetratedPoints[maxDistanceIndex] - penetratedPoints[i];
                float area = glm::determinant(glm::mat2(glm::vec2(ca[clipX], ca[clipY]), glm::vec2(cb[clipX], cb[clipY])));
                if (area > maxArea)
                {
                    maxArea      = area;
                    maxAreaIndex = i;
                }
            }

            penetratedPoint0          = penetratedPoints[maxAreaIndex];
            penetratedPoint0[boxAxis] = boxAxisSign * boxHalfExtents[boxAxis];
            contactPoints[2]          = {boxCenter + boxBasis * penetratedPoint0, boxCenter + boxBasis * penetratedPoints[maxAreaIndex]};

            float minArea      = 0;
            int   minAreaIndex = 0;
            for (int i = 0; i < penetratedPoints.size(); ++i)
            {
                auto  da   = penetratedPoints[0] - penetratedPoints[i];
                auto  db   = penetratedPoints[maxDistanceIndex] - penetratedPoints[i];
                float area = glm::determinant(glm::mat2(glm::vec2(da[clipX], da[clipY]), glm::vec2(db[clipX], db[clipY])));
                if (area < minArea)
                {
                    minArea      = area;
                    minAreaIndex = i;
                }
            }

            penetratedPoint0          = penetratedPoints[minAreaIndex];
            penetratedPoint0[boxAxis] = boxAxisSign * boxHalfExtents[boxAxis];
            contactPoints[3]          = {boxCenter + boxBasis * penetratedPoint0, boxCenter + boxBasis * penetratedPoints[minAreaIndex]};

            numPoints = 4;
        }
        else
        {
            for (int i = 0; i < penetratedPoints.size(); ++i)
            {
                glm::vec3 penetratedPoint0 = penetratedPoints[i];
                penetratedPoint0[boxAxis]  = boxAxisSign * boxHalfExtents[boxAxis];
                contactPoints[i]           = {boxCenter + boxBasis * penetratedPoint0, boxCenter + boxBasis * penetratedPoints[i]};
            }
            numPoints = penetratedPoints.size();
        }
    }

    namespace Clipping
    {
        constexpr int   MAX_POINTS = 128;
        
        static bool   isInside(glm::vec2 point, glm::vec2 a, glm::vec2 b)
        {
            return (b.y - a.y) * (point.x - a.x) - (b.x - a.x) * (point.y - a.y) > 0; // dot product with normal of edge towards interior
        }

        static glm::vec2 intersection(glm::vec2 a1, glm::vec2 b1, glm::vec2 a2, glm::vec2 b2)
        {
            auto  r1 = b1 - a1;
            auto  r2 = b2 - a2;
            float t  = glm::determinant(glm::mat2(a1 - a2, r2)) / glm::determinant(glm::mat2(-r1, r2));
            return a1 + t * r1;
        }

        static void clipEdge(std::vector<glm::vec2> &polygon, glm::vec2 a, glm::vec2 b)
        {
            glm::vec2 newPoints[MAX_POINTS];
            int       newPointCount = 0;

            for (int i = 0; i < polygon.size(); ++i)
            {
                int j = (i + 1) % polygon.size();

                bool iInside = isInside(polygon[i], a, b);
                bool jInside = isInside(polygon[j], a, b);

                if (iInside && jInside)
                {
                    newPoints[newPointCount++] = polygon[j];
                }
                else if (iInside)
                {
                    newPoints[newPointCount++] = intersection(polygon[i], polygon[j], a, b);
                }
                else if (jInside)
                {
                    newPoints[newPointCount++] = intersection(polygon[i], polygon[j], a, b);
                    newPoints[newPointCount++] = polygon[j];
                }
            }

            polygon.resize(newPointCount);

            for (int i = 0; i < newPointCount; ++i)
            {
                polygon[i] = newPoints[i];
            }
        }

        static void SuthHodglip(std::vector<glm::vec2>& polygon, std::vector<glm::vec2>& clip)
        {
            for (int i = 0; i < clip.size(); ++i)
            {
                int j = (i + 1) % clip.size();
                clipEdge(polygon, clip[i], clip[j]);
            }
        }
    }
}

namespace MamontEngine
{
    namespace HeroPhysics
    {
        static void generateBoxBoxFaceContacts(glm::mat3     uRef,
            glm::vec3     posRef,
            glm::vec3     halfExtentsRef,
            glm::mat3     uInc,
            glm::vec3     posInc,
            glm::vec3     halfExtentsInc,
            int           referenceAxis,
            ContactPoint* contactPoints,
            int& numPoints)
        {
            glm::vec3 axis = uRef[referenceAxis];
            float     refSign = glm::dot(axis, posInc - posRef) < 0 ? -1 : 1;

            float maxDot{0};
            int   incAxis = 0;
            float   incSign{0.1};
            for (int i{ 0 }; i < 3; ++i)
            {
                const float dot = glm::dot(-refSign * axis, uInc[i]);
                const float absDot = glm::abs(dot);
                if (absDot > maxDot)
                {
                    maxDot = absDot;
                    incAxis = i;
                    incSign = dot < 0 ? -1 : 1;
                }
            }

            polygon.reserve(8);
            polygon.resize(4);
            clip.resize(4);

            const int incAxis1 = (incAxis + 1) % 3, incAxis2 = (incAxis + 2) % 3;

            glm::mat3 inverseURef = glm::inverse(uRef);

            const glm::vec3 incPlaneOrig = inverseURef * (posInc + uInc[incAxis] * halfExtentsInc[incAxis] * incSign - posRef);

            const glm::vec3 poly0 = incPlaneOrig + inverseURef * (uInc[incAxis1] * halfExtentsInc[incAxis1] + uInc[incAxis2] * halfExtentsInc[incAxis2]);
            const glm::vec3 poly1 = incPlaneOrig + inverseURef * (-uInc[incAxis1] * halfExtentsInc[incAxis1] + uInc[incAxis2] * halfExtentsInc[incAxis2]);
            const glm::vec3 poly2 = incPlaneOrig + inverseURef * (-uInc[incAxis1] * halfExtentsInc[incAxis1] - uInc[incAxis2] * halfExtentsInc[incAxis2]);
            const glm::vec3 poly3 = incPlaneOrig + inverseURef * (uInc[incAxis1] * halfExtentsInc[incAxis1] - uInc[incAxis2] * halfExtentsInc[incAxis2]);

            int clipX = (referenceAxis + 1) % 3;
            int clipY = (referenceAxis + 2) % 3;

            polygon[0] = {poly0[clipX], poly0[clipY]};
            polygon[1] = {poly1[clipX], poly1[clipY]};
            polygon[2] = {poly2[clipX], poly2[clipY]};
            polygon[3] = {poly3[clipX], poly3[clipY]};

            clip[0] = {halfExtentsRef[clipX], halfExtentsRef[clipY]};
            clip[1] = {halfExtentsRef[clipX], -halfExtentsRef[clipY]};
            clip[2] = {-halfExtentsRef[clipX], -halfExtentsRef[clipY]};
            clip[3] = {-halfExtentsRef[clipX], halfExtentsRef[clipY]};

            Clipping::SuthHodglip(polygon, clip);

            const glm::vec3 incPlaneNormal = inverseURef * uInc[incAxis];

            generateContactsPolygonBoxFace(
                    posRef, uRef, referenceAxis, refSign, halfExtentsRef, incPlaneOrig, incPlaneNormal, polygon, clipX, clipY, contactPoints, numPoints);
        }

        static bool CheckCollisionBoxBox(const CollisionPair *inPair, ContactManifold &outResults)
        {
            float edgeOffset{.1f};
            constexpr float edgeLimit{.999f};

            float max = std::numeric_limits<float>::lowest();

            EBoxContactType contactType = EBoxContactType::FACE;
            BoxEdgeContactInfo edgeInfo;
            BoxFaceContactInfo faceInfo;

            glm::mat3 r = glm::mat3();
            glm::mat3 absR = glm::mat3();

            glm::mat3 u0 = glm::mat3(inPair->Object1->GetRotation());
            glm::mat3 u1 = glm::mat3(inPair->Object2->GetRotation());

            const glm::vec3 position1 = inPair->Object1->GetPosition();
            const glm::vec3 position2 = inPair->Object2->GetPosition();

            glm::vec3 t = position1 - position2;
            t           = glm::vec3(glm::dot(t, u0[0]), glm::dot(t, u0[1]), glm::dot(t, u0[2]));

            const auto            &shapeObject1 = inPair->Object1->GetShape();
            const auto           &shapeObject2 = inPair->Object2->GetShape();
            const auto            &shape1       = std::static_pointer_cast<BoxCollision>(shapeObject1);
            const auto            &shape2       = std::static_pointer_cast<BoxCollision>(shapeObject2);
            const glm::vec3       halfExtents1 = shape1->GetHalfExtent();
            const glm::vec3       halfExtents2 = shape2->GetHalfExtent();

            float ra = 0.f, rb = 0.f, l = 0.f, d = 0.f;

            for (int i{ 0 }; i < 3; ++i)
            {
                for (int j{ 0 }; j < 3; ++j)
                {
                    r[i][j] = glm::dot(u0[i], u1[j]);
                    absR[i][j] = glm::abs(r[i][j]);
                }
            }

            for (int i{ 0 }; i < 3; i++)
            {
                ra = halfExtents1[i];
                rb = halfExtents2[0] * absR[i][0] + halfExtents2[1] * absR[i][1] + halfExtents2[2] * absR[i][2];
                l  = glm::abs(t[i]);
                d  = l - ra - rb;
                if (d > 0) return false;

                if (d > max)
                {
                    max = d;
                    contactType = EBoxContactType::FACE;
                    faceInfo    = {0, i};
                }
            }

            for (int i{ 0 }; i < 3; i++)
            {
                ra = halfExtents1[0] * absR[0][i] + halfExtents1[1] * absR[1][i] + halfExtents1[2] * absR[2][i];
                rb = halfExtents2[i];
                l  = glm::abs(t[0] * r[0][i] + t[1] * r[1][i] + t[2] * r[2][i]);
                d  = l - ra - rb;

                if (d > 0) return false;
                if (d > max)
                {
                    max = d;
                    contactType = EBoxContactType::FACE;
                    faceInfo    = {1, i};
                }
            }

            // Test axis L = A0 x B0
            ra = halfExtents1[1] * absR[2][0] + halfExtents1[2] * absR[1][0];
            rb = halfExtents2[1] * absR[0][2] + halfExtents2[2] * absR[0][1];
            l  = glm::abs(t[2] * r[1][0] - t[1] * r[2][0]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[0][0] < edgeLimit && d > max + edgeOffset)
            {
                max = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {0, 0};
                edgeOffset  = 0;
            }

            // Test exis L = A0 x B1
            ra = halfExtents1[1] * absR[2][1] + halfExtents1[2] * absR[1][1];
            rb = halfExtents2[0] * absR[0][2] + halfExtents2[2] * absR[0][0];
            l  = glm::abs(t[2] * r[1][1] - t[1] * r[2][1]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[0][1] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {0, 1};
                edgeOffset  = 0;
            }

            // Test exis L = A0 x B2
            ra = halfExtents1[1] * absR[2][2] + halfExtents1[2] * absR[1][2];
            rb = halfExtents2[0] * absR[0][1] + halfExtents2[1] * absR[0][0];
            l  = glm::abs(t[2] * r[1][2] - t[1] * r[2][2]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[0][2] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {0, 2};
                edgeOffset  = 0;
            }

            // Test exis L = A1 x B0
            ra = halfExtents1[0] * absR[2][0] + halfExtents1[2] * absR[0][0];
            rb = halfExtents2[1] * absR[1][2] + halfExtents2[2] * absR[1][1];
            l  = glm::abs(t[0] * r[2][0] - t[2] * r[0][0]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[1][0] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {1, 0};
                edgeOffset  = 0;
            }

            // Test exis L = A1 x B1
            ra = halfExtents1[0] * absR[2][1] + halfExtents1[2] * absR[0][1];
            rb = halfExtents2[0] * absR[1][2] + halfExtents2[2] * absR[1][0];
            l  = glm::abs(t[0] * r[2][1] - t[2] * r[0][1]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[1][1] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {1, 1};
                edgeOffset  = 0;
            }

            // Test exis L = A1 x B2
            ra = halfExtents1[0] * absR[2][2] + halfExtents1[2] * absR[0][2];
            rb = halfExtents2[0] * absR[1][1] + halfExtents2[1] * absR[1][0];
            l  = glm::abs(t[0] * r[2][2] - t[2] * r[0][2]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[1][2] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {1, 2};
                edgeOffset  = 0;
            }

            // Test exis L = A2 x B0
            ra = halfExtents1[0] * absR[1][0] + halfExtents1[1] * absR[0][0];
            rb = halfExtents2[1] * absR[2][2] + halfExtents2[2] * absR[2][1];
            l  = glm::abs(t[1] * r[0][0] - t[0] * r[1][0]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[2][0] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {2, 0};
                edgeOffset  = 0;
            }

            // Test exis L = A2 x B1
            ra = halfExtents1[0] * absR[1][1] + halfExtents1[1] * absR[0][1];
            rb = halfExtents2[0] * absR[2][2] + halfExtents2[2] * absR[2][0];
            l  = glm::abs(t[1] * r[0][1] - t[0] * r[1][1]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[2][1] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {2, 1};
                edgeOffset  = 0;
            }

            // Test exis L = A2 x B2
            ra = halfExtents1[0] * absR[1][2] + halfExtents1[1] * absR[0][2];
            rb = halfExtents2[0] * absR[2][1] + halfExtents2[1] * absR[2][0];
            l  = glm::abs(t[1] * r[0][2] - t[0] * r[1][2]);
            d  = l - ra - rb;
            if (d > 0) return false;
            if (absR[2][2] < edgeLimit && d > max + edgeOffset)
            {
                max         = d;
                contactType = EBoxContactType::EDGE;
                edgeInfo    = {2, 2};
            }

            switch (contactType)
            {
                case EBoxContactType::FACE:
                {
                    glm::vec3 axis;
                    ContactPoint contactPoints[4];
                    int          numPoints;
                    if (faceInfo.Box)
                    {
                        axis = u1[faceInfo.Axis];
                        generateBoxBoxFaceContacts(u1, position2, halfExtents2, u0, position1, halfExtents1, faceInfo.Axis, contactPoints, numPoints);
                        
                        for (int i{ 0 }; i < numPoints; i++) 
                        {
                            outResults.Points[i] = {contactPoints[i].Position1, contactPoints[i].Position2};
                        }
                    }
                    else
                    {
                        axis = u0[faceInfo.Axis];
                        generateBoxBoxFaceContacts(u0, position1, halfExtents1, u1, position2, halfExtents2, faceInfo.Axis, contactPoints, numPoints);
                        for (int i{0}; i < numPoints; i++)
                        {
                            outResults.Points[i] = {contactPoints[i].Position1, contactPoints[i].Position2};
                        }
                    }

                    outResults.NumPoints = numPoints;
                    outResults.Normal    = glm::dot(axis, position2 - position1) < 0 ? -axis : axis;
                }
                break;
                
                case EBoxContactType::EDGE:
                {
                    glm::vec3 axis = glm::cross(u0[edgeInfo.edge0], u1[edgeInfo.edge1]);
                    outResults.Normal = glm::normalize(glm::dot(axis, position2 - position1) < 0 ? -axis : axis);
                    glm::vec3 sign0(-1);
                    sign0[(edgeInfo.edge0 + 1) % 3] = glm::dot(u0[(edgeInfo.edge0 + 1) % 3], outResults.Normal) < 0 ? -1 : 1;
                    sign0[(edgeInfo.edge0 + 2) % 3] = glm::dot(u0[(edgeInfo.edge0 + 2) % 3], outResults.Normal) < 0 ? -1 : 1;
                    glm::vec3 p0 = position1 + sign0[0] * halfExtents1[0] * u0[0] + sign0[1] * halfExtents1[1] * u0[1] + sign0[2] * halfExtents1[2] * u0[2];
                    glm::vec3 sign1(-1);
                    sign1[(edgeInfo.edge1 + 1) % 3] = glm::dot(u1[(edgeInfo.edge1 + 1) % 3], outResults.Normal) > 0 ? -1 : 1;
                    sign1[(edgeInfo.edge1 + 2) % 3] = glm::dot(u1[(edgeInfo.edge1 + 2) % 3], outResults.Normal) > 0 ? -1 : 1;
                    glm::vec3 p1          = position2 + sign1[0] * halfExtents2[0] * u1[0] + sign1[1] * halfExtents2[1] * u1[1] + sign1[2] * halfExtents2[2] * u1[2];
                    outResults.NumPoints= 1;
                    auto [point0, point1] = closestPointsBetweenSegments(
                            p0, u0[edgeInfo.edge0], 0, halfExtents1[edgeInfo.edge0] * 2, p1, u1[edgeInfo.edge1], 0, halfExtents2[edgeInfo.edge1] * 2);
                    outResults.Points[0] = {point0, point1};
                }
                break;
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
           /* const auto &shape1 = inPair->Object1->GetShape();
            const auto &shape2 = inPair->Object2->GetShape();

            if (shape1->GetShapeType() == EShapeType::Sphere)
            {
                if (shape2->GetShapeType() == EShapeType::Sphere)
                {
                    return CheckSpherSphere(inPair, collisionData);
                }
            }*/

            return CheckSpherSphere(inPair, collisionData);
        }

        /*        bool CheckCollision(const CollisionPair *inPair, std::vector<ContactManifold> &outResults)
        {
            outResults.push_back(ContactManifold());

            return CheckCollisionBoxBox(inPair, outResults[0]);
            //return false;
        }*/
       /* bool CheckCollision(const CollisionPair *inPair, CollisionData *outCollisionData)
        {
            PROFILE_FUNCTION();

            CollisionData bestColData;
            bestColData.Penetration = -FLT_MAX;

            const auto shape1Axes = GetCollisionAxes(inPair->Object1->GetRotation());
            const auto shape2Axes = GetCollisionAxes(inPair->Object2->GetRotation());

            static constexpr int MAX_COLLISION_AXES = 100;
            glm::vec3            possibleCollisionAxes[MAX_COLLISION_AXES];

            uint32_t possibleAxesCount{0};

            for (const glm::vec3& axis : shape1Axes)
            {
                possibleCollisionAxes[possibleAxesCount++] = axis;
            }

            for (const glm::vec3& axis : shape2Axes)
            {
                possibleCollisionAxes[possibleAxesCount++] = axis;
            }

            return false;
        }*/
    } // namespace HeroPhysics
} // namespace MamontEngine