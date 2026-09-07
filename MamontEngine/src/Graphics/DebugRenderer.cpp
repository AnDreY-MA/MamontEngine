#include "Graphics/DebugRenderer.h"
#include <Utils/Profile.h>
#include "Core/Log.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Graphics/Resources/Models/Mesh.h"
#include "Graphics/Vulkan/Allocator.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Graphics/Vulkan/Pipelines/PipelineData.h"
#include "Utils//VkPipelines.h"
#include "Utils//VkDestriptor.h"
#include "Utils//VkInitializers.h"

constexpr uint32_t MAX_VERTEX_COUNT = 1024;

namespace MamontEngine
{
    namespace DebugRenderer
    {
        std::vector<Vertex> g_Vertices;
        AllocatedBuffer     g_Buffer;
        VkDeviceAddress     g_BufferAddress;
        uint32_t            m_VerticesCount{0};
        std::unique_ptr<PipelineData> DebugDrawPipeline;

        void Init(VkDevice inDevice, std::span<const VkDescriptorSetLayout> inDescriptorLayouts)
        {
            g_Buffer.Create(sizeof(Vertex) * MAX_VERTEX_COUNT,
                            VK_BUFFER_USAGE_2_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT,
                            VMA_MEMORY_USAGE_CPU_TO_GPU);
            const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = g_Buffer.Buffer};
            g_BufferAddress                                   = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);

            const std::string debugDrawPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/debug_draw.frag.spv";

            VkShaderModule debugDrawFragShader;
            if (!VkPipelines::LoadShaderModule(debugDrawPath.c_str(), inDevice, &debugDrawFragShader))
            {
                fmt::println("Error when building the triangle fragment shader module");
            }

            const std::string debugDrawVertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/debug_draw.vert.spv";
            VkShaderModule    debugDrawVertexShader;
            if (!VkPipelines::LoadShaderModule(debugDrawVertexShaderPath.c_str(), inDevice, &debugDrawVertexShader))
            {
                fmt::println("Error when building the triangle vertex shader module");
            }
            const std::vector<VkVertexInputBindingDescription>   vertexInputBindings{};
            const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
            const VkPipelineVertexInputStateCreateInfo           vertexInputInfo =
                    vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);


            constexpr VkPushConstantRange matrixRange{
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};

            const VkPipelineLayoutCreateInfo layoutInfo =
                    vkinit::pipeline_layout_create_info(inDescriptorLayouts.size(), inDescriptorLayouts.data(), &matrixRange, 1);

            VkPipelines::PipelineBuilder pipelineBuilder;

            pipelineBuilder.SetShaders(debugDrawVertexShader, debugDrawFragShader);
            VkPipelineLayout debugLayout;
            VK_CHECK(vkCreatePipelineLayout(inDevice, &layoutInfo, nullptr, &debugLayout));
            pipelineBuilder.SetLayout(debugLayout);
            pipelineBuilder.SetVertexInput(vertexInputInfo);
            pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_LINE_LIST);
            pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
            pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pipelineBuilder.EnableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
            pipelineBuilder.m_DepthStencil.depthWriteEnable = VK_FALSE;
            DebugDrawPipeline                               = std::make_unique<PipelineData>(pipelineBuilder.BuildPipline(inDevice), debugLayout);

            vkDestroyShaderModule(inDevice, debugDrawFragShader, nullptr);
            vkDestroyShaderModule(inDevice, debugDrawVertexShader, nullptr);
        }

        void Destroy()
        {
            g_Vertices.clear();
            g_Buffer.Destroy();
            DebugDrawPipeline.reset();
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

        void Render(VkCommandBuffer inCmd)
        {
            if (const auto vertexCount = DebugRenderer::GetVertexCount(); vertexCount > 0)
            {
                constexpr uint32_t constantsSize{static_cast<uint32_t>(sizeof(GPUDrawPushConstants))};

                vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, DebugDrawPipeline->Pipeline);

                const auto                 bufferAdress = DebugRenderer::GetVertexBufferAdress();
                const GPUDrawPushConstants debug_pushconstants{
                        .WorldMatrix  = glm::mat4(1.f),
                        .VertexBuffer = bufferAdress,
                };

                vkCmdPushConstants(inCmd,
                                   DebugDrawPipeline->Layout,
                                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                                   0,
                                   constantsSize,
                                   &debug_pushconstants);
                vkCmdDraw(inCmd, vertexCount, 1, 0, 0);
            }
        }

        void ClearVertex()
        {
            m_VerticesCount = 0;
            g_Vertices.clear();
        }

        void DrawPoint(const glm::vec3 inPostion, const float radius, const Color &inColor)
        {
            const auto pointVertex = Vertex{.Position = inPostion, .Color = inColor.ToVector4() * 1.f};
            g_Vertices.push_back(pointVertex);
        }

        void DrawLine(const glm::vec3 &inStart, const glm::vec3 &inEnd, const Color &inColor, float inWidth)
        {
            const auto startVertex = Vertex{.Position = inStart, .Color = inColor.ToVector4() * inWidth};
            g_Vertices.push_back(startVertex);
            const auto endVertex = Vertex{.Position = inEnd, .Color = inColor.ToVector4() * inWidth};
            g_Vertices.push_back(endVertex);
        }

        void Draw(const AABB &inBox, const Color &inColor, float width)
        {
            PROFILE_FUNCTION();

            const glm::vec3 &min = inBox.Min;
            const glm::vec3 &max = inBox.Max;

            const glm::vec3 v[8] = {{min.x, min.y, min.z},
                                    {max.x, min.y, min.z},
                                    {max.x, max.y, min.z},
                                    {min.x, max.y, min.z},
                                    {min.x, min.y, max.z},
                                    {max.x, min.y, max.z},
                                    {max.x, max.y, max.z},
                                    {min.x, max.y, max.z}};

            constexpr int edges[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};

            for (const auto &e : edges)
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
