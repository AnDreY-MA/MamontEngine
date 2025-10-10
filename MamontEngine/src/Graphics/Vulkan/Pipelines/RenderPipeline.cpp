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

    RenderPipeline::RenderPipeline(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, const std::pair<VkFormat, VkFormat> inImageFormats)
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

        constexpr VkPushConstantRange matrixRange {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            .offset = 0, 
            .size = sizeof(GPUDrawPushConstants)
        };

        DescriptorLayoutBuilder layoutBuilder{};
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        Layout = layoutBuilder.Build(inDevice, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        const VkDescriptorSetLayout layouts[] = {inDescriptorLayout, Layout};

        VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
        mesh_layout_info.setLayoutCount             = 2;
        mesh_layout_info.pSetLayouts                = layouts;
        mesh_layout_info.pPushConstantRanges        = &matrixRange;
        mesh_layout_info.pushConstantRangeCount     = 1;

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &newLayout));

        /*const std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
                vkinit::vertex_input_binding_description(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX),
        };
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
                vkinit::vertex_input_attribute_description(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Position)),
                vkinit::vertex_input_attribute_description(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Normal)),
                vkinit::vertex_input_attribute_description(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, UV)),
                vkinit::vertex_input_attribute_description(0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Color)),
                vkinit::vertex_input_attribute_description(0, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, Tangent)),
        };*/
        const std::vector<VkVertexInputBindingDescription>  vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);
        pipelineBuilder.SetVertexInput(vertexInputInfo);
        pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
        pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
        pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        pipelineBuilder.SetColorAttachmentFormat(inImageFormats.first);
        pipelineBuilder.SetDepthFormat(inImageFormats.second);

        pipelineBuilder.SetLayout(newLayout);

        OpaquePipeline          = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), newLayout);

        //Transparent
        pipelineBuilder.EnableBlendingAdditive();
        pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

        TransparentPipeline          = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), newLayout);

        vkDestroyShaderModule(inDevice, meshFragShader, nullptr);
        vkDestroyShaderModule(inDevice, meshVertexShader, nullptr);
    }

    void RenderPipeline::Destroy(VkDevice inDevice)
    {
        /*if (OpaquePipeline == nullptr)
        {
            fmt::println("OpaquePipeline == nullptr");
        }
        vkDestroyPipelineLayout(inDevice, OpaquePipeline->Layout, nullptr);
        vkDestroyPipeline(inDevice, OpaquePipeline->Pipeline, nullptr);
        if (TransparentPipeline == nullptr)
        {
            fmt::println("TransparentPipeline == nullptr");
        }
        vkDestroyPipelineLayout(inDevice, TransparentPipeline->Layout, nullptr);
        vkDestroyPipeline(inDevice, TransparentPipeline->Pipeline, nullptr);*/
    }
    
} // namespace MamontEngine