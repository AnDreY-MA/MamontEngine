#pragma once 

namespace MamontEngine
{
    namespace HeroPhysics
    {
        struct ContactPoint
        {
            glm::vec3 Position1;
            glm::vec3 Position2;
        };

        struct ContactManifold
        {
            int NumPoints{0};
            ContactPoint Points[4];
            glm::vec3    Normal{glm::vec3(0.)};
            int          TriangleIndex{-1};
        };

        struct ContactPointData
        {
            glm::vec3 LocalPosition{glm::vec3(0.f)};
            float     TargetVelocity{0.f};
        };

        struct ContactManifoldData
        {
            int NumPoints{0};
            ContactPointData ContactData[4];
        };
    }
} // namespace MamontEngine
