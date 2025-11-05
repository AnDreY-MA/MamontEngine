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
    RenderPipeline::RenderPipeline(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, const std::pair<VkFormat, VkFormat> inImageFormats)
    {
        const std::string meshPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/mesh.frag.spv";

        VkShaderModule meshFragShader;
        if (!VkPipelines::LoadShaderModule(meshPath.c_str(), inDevice, &meshFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string meshVertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/mesh.vert.spv";
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

        const std::array<VkDescriptorSetLayout, 2> layouts = {inDescriptorLayout, Layout};

        const VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info(2, layouts.data(), &matrixRange, 1);

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
        pipelineBuilder.SetCullMode(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
        pipelineBuilder.SetMultisamplingNone();
        pipelineBuilder.DisableBlending();
        pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.SetColorAttachmentFormat(inImageFormats.first);
        pipelineBuilder.SetDepthFormat(inImageFormats.second);
        pipelineBuilder.SetLayout(newLayout);

        const VkPipeline opaquePipeline = pipelineBuilder.BuildPipline(inDevice);
        OpaquePipeline            = std::make_unique<PipelineData>(opaquePipeline, newLayout);

        std::cerr << "OpaquePipeline->Pipeline: " << OpaquePipeline->Pipeline << std::endl;
        std::cerr << "OpaquePipeline->Pipeline, newLayout: " << newLayout << std::endl;

        //Transparent
        pipelineBuilder.EnableBlendingAdditive();
        //pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

        TransparentPipeline          = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), newLayout);
        std::cerr << "TransparentPipeline->Pipeline: " << TransparentPipeline->Pipeline << std::endl;

        vkDestroyShaderModule(inDevice, meshFragShader, nullptr);
        vkDestroyShaderModule(inDevice, meshVertexShader, nullptr);
    }

    RenderPipeline::~RenderPipeline()
    {
        OpaquePipeline.reset();
        TransparentPipeline.reset();
    }
    
} // namespace MamontEngine