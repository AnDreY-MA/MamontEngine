#include "Graphics/DebugRenderer.h"
#include <Utils/Profile.h>
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Core/Log.h"

constexpr uint32_t MAX_VERTEX_COUNT = 1024;

namespace MamontEngine
{
    namespace DebugRenderer
    {
        std::vector<Vertex>   g_Vertices;
        AllocatedBuffer       g_Buffer;
        VkDeviceAddress             g_BufferAddress;
        uint32_t              m_VerticesCount{0};

        void Init()
        {
            g_Buffer.Create(MAX_VERTEX_COUNT * sizeof(Vertex),
                            VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU);
            const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = g_Buffer.Buffer};
            g_BufferAddress                                   = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);
        }

        void Destroy()
        {
            g_Vertices.clear();
            g_Buffer.Destroy();
        }

        void Update()
        {
            m_VerticesCount = static_cast<uint32_t>(g_Vertices.size());
            if (m_VerticesCount > 0)
            {
                const VkDeviceSize bufferSize = m_VerticesCount * sizeof(Vertex);
                g_Buffer.Copy(g_Vertices.data(), bufferSize);
            }
        }

        void ClearVertex()
        {
            m_VerticesCount = 0;
            g_Vertices.clear();
        }

        void DrawPoint(const glm::vec3 inPostion, const float radius, const Color& inColor)
        {
            const auto pointVertex = Vertex{.Position = inPostion, .Color = inColor.ToVector4() * 50.f};
            g_Vertices.push_back(pointVertex);
        }

        void DrawLine(const glm::vec3& inStart, const glm::vec3& inEnd, const Color& inColor, float inWidth)
        {
            const auto startVertex = Vertex{.Position = inStart, .Color = inColor.ToVector4()};
            g_Vertices.push_back(startVertex);
            const auto endVertex = Vertex{.Position = inEnd, .Color = inColor.ToVector4()};
            g_Vertices.push_back(endVertex);
        }

        void Draw(const AABB &inBox, const Color &inColor, float width)
        {
            PROFILE_FUNCTION();

            const glm::vec3& min = inBox.Min;
            const glm::vec3& max = inBox.Max;

            const glm::vec3 v[8] = {
                    {min.x, min.y, min.z}, 
                    {max.x, min.y, min.z}, 
                    {max.x, max.y, min.z}, 
                    {min.x, max.y, min.z}, 
                    {min.x, min.y, max.z}, 
                    {max.x, min.y, max.z}, 
                    {max.x, max.y, max.z}, 
                    {min.x, max.y, max.z}
            };

            constexpr int edges[12][2] = {
                {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

            for (const auto& e : edges)
            {
                DrawLine(v[e[0]], v[e[1]], inColor, width);
            }
        }

        VkDeviceAddress GetVertexBufferAdress()
        {
            return g_BufferAddress;
        }

        uint32_t GetVertexCount()
        {
            return m_VerticesCount;
        }
    } // namespace DebugRenderer
} // namespace MamontEngine
