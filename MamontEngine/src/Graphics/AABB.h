#pragma once

namespace MamontEngine
{
	class AABB
	{
    public:
        explicit AABB() = default;
        ~AABB() = default;

        AABB(const glm::vec3 &min, const glm::vec3 &max) : Min(glm::min(min, max)), Max(glm::max(min, max))
        {
            assert(Min.x <= Max.x && Min.y <= Max.y && Min.z <= Max.z);
        }

        void Reset()
        {
            Min = glm::vec3(FLT_MAX);
            Max = glm::vec3(-FLT_MAX);
        }

        glm::vec3 Extent() const
        {
            return Max - Min;
        }

        void Expand(const AABB &other)
        {
            Min = glm::min(Min, other.Min);
            Max = glm::max(Max, other.Max);
        }

        // Расширяет AABB точкой
        void Expand(const glm::vec3 &point)
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }

        float Radius() const
        {
            return glm::length(Extent()) * 0.5f;
        }

        glm::vec3 Center() const
        {
            return (Min + Max) * 0.5f;
        }

        // Трансформирует AABB матрицей (например, для world-space)
        AABB Transform(const glm::mat4 &m) const
        {
            const glm::vec3 corners[8] = {{Min.x, Min.y, Min.z},
                                    {Max.x, Min.y, Min.z},
                                    {Min.x, Max.y, Min.z},
                                    {Max.x, Max.y, Min.z},
                                    {Min.x, Min.y, Max.z},
                                    {Max.x, Min.y, Max.z},
                                    {Min.x, Max.y, Max.z},
                                    {Max.x, Max.y, Max.z}};

            AABB result;
            result.Reset();

            for (const auto &c : corners)
            {
                glm::vec3 world = glm::vec3(m * glm::vec4(c, 1.0f));
                result.Expand(world);
            }
            return result;
        }

        // Проверка пересечения с лучом
        bool IntersectsRay(const glm::vec3 &origin, const glm::vec3 &dir, float &outDist) const
        {
            float tmin = 0.0f;
            float tmax = FLT_MAX;

            for (int i = 0; i < 3; i++)
            {
                if (std::abs(dir[i]) < 1e-8f)
                {
                    if (origin[i] < Min[i] || origin[i] > Max[i])
                        return false;
                }
                else
                {
                    float invD = 1.0f / dir[i];
                    float t1   = (Min[i] - origin[i]) * invD;
                    float t2   = (Max[i] - origin[i]) * invD;
                    if (t1 > t2)
                        std::swap(t1, t2);

                    tmin = std::max(tmin, t1);
                    tmax = std::min(tmax, t2);

                    if (tmax < tmin)
                        return false;
                }
            }

            outDist = tmin;
            return true;
        }

        glm::vec3 Min = glm::vec3(FLT_MAX);
        glm::vec3 Max = glm::vec3(-FLT_MAX);
    };


       
}