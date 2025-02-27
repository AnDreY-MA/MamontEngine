#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"

#include "Core/VkPipelines.h"
#include <Core/VkDestriptor.h>
#include <Core/VkInitializers.h>
#include "Core/ContextDevice.h"
#include <ECS/SceneRenderer.h>


//#define VMA_IMPLEMENTATION
//#include <vk_mem_alloc.h>


namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void RenderPipeline::Init(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, std::pair<VkFormat, VkFormat> inImageFormats)
    {
        const std::string meshPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.frag.spv";

        VkShaderModule meshFragShader;
        if (!VkPipelines::LoadShaderModule(meshPath.c_str(), inDevice, &meshFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string meshVertexShaderPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.vert.spv";
        VkShaderModule    meshVertexShader;
        if (!VkPipelines::LoadShaderModule(meshVertexShaderPath.c_str(), inDevice, &meshVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }

        const VkPushConstantRange matrixRange{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};

        DescriptorLayoutBuilder layoutBuilder{};
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        Layout = layoutBuilder.Build(inDevice, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        const VkDescriptorSetLayout layouts[] = {inDescriptorLayout, Layout};

        VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
        mesh_layout_info.setLayoutCount             = 2;
        mesh_layout_info.pSetLayouts                = layouts;
        mesh_layout_info.pPushConstantRanges        = &matrixRange;
        mesh_layout_info.pushConstantRangeCount     = 1;

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &newLayout));

        OpaquePipeline.Layout      = newLayout;
        TransparentPipeline.Layout = newLayout;

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        pipelineBuilder.SetColorAttachmentFormat(inImageFormats.first);
        pipelineBuilder.SetDepthFormat(inImageFormats.second);

        pipelineBuilder.m_PipelineLayout = newLayout;

        OpaquePipeline.Pipeline = pipelineBuilder.BuildPipline(inDevice);

        pipelineBuilder.EnableBlendingAdditive();

        pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

        TransparentPipeline.Pipeline = pipelineBuilder.BuildPipline(inDevice);

        vkDestroyShaderModule(inDevice, meshFragShader, nullptr);
        vkDestroyShaderModule(inDevice, meshVertexShader, nullptr);
    }

    static bool is_visible(const RenderObject &obj, const glm::mat4 &viewproj)
    {
        const std::array<glm::vec3, 8> corners{
                glm::vec3{1, 1, 1},
                glm::vec3{1, 1, -1},
                glm::vec3{1, -1, 1},
                glm::vec3{1, -1, -1},
                glm::vec3{-1, 1, 1},
                glm::vec3{-1, 1, -1},
                glm::vec3{-1, -1, 1},
                glm::vec3{-1, -1, -1},
        };

        const glm::mat4 matrix = viewproj * obj.Transform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            glm::vec4 v = matrix * glm::vec4(obj.Bound.Origin + (corners[c] * obj.Bound.Extents), 1.f);

            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        if (min.z > 1.f || max.z < 0.f || min.x > 1.f || max.x < -1.f || min.y > 1.f || max.y < -1.f)
        {
            return false;
        }

        return true;
    }

    void RenderPipeline::Draw(VkCommandBuffer inCmd, const VkDescriptorSet& globalDescriptor,
                              GPUSceneData& inSceneData,
                              const VkExtent2D &inDrawExtent)
    {
        std::vector<uint32_t> opaque_draws;
        opaque_draws.reserve(MainDrawContext.OpaqueSurfaces.size());

        for (int i = 0; i < MainDrawContext.OpaqueSurfaces.size(); i++)
        {
            if (is_visible(MainDrawContext.OpaqueSurfaces[i], inSceneData.Viewproj))
            {
                opaque_draws.push_back(i);
            }
        }

        std::vector<uint32_t> transp_draws;
        transp_draws.reserve(MainDrawContext.TransparentSurfaces.size());

        for (int i = 0; i < MainDrawContext.TransparentSurfaces.size(); i++)
        {
            if (is_visible(MainDrawContext.TransparentSurfaces[i], inSceneData.Viewproj))
            {
                transp_draws.push_back(i);
            }
        }

        MaterialPipeline *lastPipeline    = nullptr;
        MaterialInstance *lastMaterial    = nullptr;
        VkBuffer          lastIndexBuffer = VK_NULL_HANDLE;

        auto draw = [&](const RenderObject &r)
        {
            if (r.Material != lastMaterial)
            {
                lastMaterial = r.Material;
                if (r.Material->Pipeline != lastPipeline)
                {
                    lastPipeline = r.Material->Pipeline;
                    vkCmdBindPipeline(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Pipeline);

                    vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 0, 1, &globalDescriptor, 0, nullptr);

                    const VkViewport viewport = {0, 0, (float)inDrawExtent.width, (float)inDrawExtent.height, 0.f, 1.f};

                    vkCmdSetViewport(inCmd, 0, 1, &viewport);

                    const VkRect2D scissor = {0, 0, inDrawExtent.width, inDrawExtent.height};
                    vkCmdSetScissor(inCmd, 0, 1, &scissor);
                }

                vkCmdBindDescriptorSets(inCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.Material->Pipeline->Layout, 1, 1, &r.Material->MaterialSet, 0, nullptr);
            }
            if (r.IndexBuffer != lastIndexBuffer)
            {
                lastIndexBuffer = r.IndexBuffer;
                vkCmdBindIndexBuffer(inCmd, r.IndexBuffer, 0, VK_INDEX_TYPE_UINT32);
            }

            const GPUDrawPushConstants push_constants{r.Transform, r.VertexBufferAddress};

            vkCmdPushConstants(inCmd, r.Material->Pipeline->Layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GPUDrawPushConstants), &push_constants);

            vkCmdDrawIndexed(inCmd, r.IndexCount, 1, r.FirstIndex, 0, 0);
        };


        for (auto &r : opaque_draws)
        {
            draw(MainDrawContext.OpaqueSurfaces[r]);
        }

        for (auto &r : transp_draws)
        {
            draw(MainDrawContext.TransparentSurfaces[r]);
        }

        MainDrawContext.Clear();
    }
} // namespace MamontEngine