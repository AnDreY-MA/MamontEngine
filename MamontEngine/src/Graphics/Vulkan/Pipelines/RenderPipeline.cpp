#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"

#include "Utils//VkPipelines.h"
#include "Utils//VkDestriptor.h"
#include "Utils//VkInitializers.h"
#include "Core/ContextDevice.h"
#include "ECS/SceneRenderer.h"

//#define VMA_IMPLEMENTATION
//#include <vk_mem_alloc.h>

namespace MamontEngine
{
    RenderPipeline::RenderPipeline(VkDevice                              inDevice,
                                   const std::span<VkDescriptorSetLayout> inDescriptorLayouts,
                                   const std::pair<VkFormat, VkFormat>   inImageFormats)
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

        const VkPipelineLayoutCreateInfo mesh_layout_info =
                vkinit::pipeline_layout_create_info(inDescriptorLayouts.size(), inDescriptorLayouts.data(), &matrixRange, 1);

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &newLayout));

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
        pipelineBuilder.EnableDepthTest(VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
        pipelineBuilder.SetColorAttachmentFormat(inImageFormats.first);
        pipelineBuilder.SetDepthFormat(inImageFormats.second);
        pipelineBuilder.SetLayout(newLayout);

        const VkPipeline opaquePipeline = pipelineBuilder.BuildPipline(inDevice);
        OpaquePipeline            = std::make_unique<PipelineData>(opaquePipeline, newLayout);

        std::cerr << "OpaquePipeline->Pipeline: " << OpaquePipeline->Pipeline << std::endl;
        std::cerr << "OpaquePipeline->Pipeline, newLayout: " << newLayout << std::endl;


        //Transparent
        {
            VkPipelineLayout transparentLayout;
            VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &transparentLayout));
            pipelineBuilder.SetLayout(transparentLayout);
            pipelineBuilder.EnableBlendingAdditive();

            TransparentPipeline = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), transparentLayout);
            std::cerr << "TransparentPipeline->Pipeline: " << TransparentPipeline->Pipeline << std::endl;
            std::cerr << "TransparentPipeline->Pipeline, layout: " << TransparentPipeline->Layout << std::endl;
        }
        

        vkDestroyShaderModule(inDevice, meshFragShader, nullptr);
        vkDestroyShaderModule(inDevice, meshVertexShader, nullptr);

        //Skybox

       {
            const VkPipelineLayoutCreateInfo skyboxlayoutInfo =
                    vkinit::pipeline_layout_create_info(inDescriptorLayouts.size(), inDescriptorLayouts.data(), &matrixRange, 1);

            VkPipelineLayout skyboxLayout;
            VK_CHECK(vkCreatePipelineLayout(inDevice, &skyboxlayoutInfo, nullptr, &skyboxLayout));

            const std::string skyboxPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/skybox.frag.spv";

            VkShaderModule skyboxFragShader;
            if (!VkPipelines::LoadShaderModule(skyboxPath.c_str(), inDevice, &skyboxFragShader))
            {
                fmt::println("Error when building the triangle fragment shader module");
            }

            const std::string skyboxVertexShaderPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/skybox.vert.spv";
            VkShaderModule    skyboxVertexShader;
            if (!VkPipelines::LoadShaderModule(skyboxVertexShaderPath.c_str(), inDevice, &skyboxVertexShader))
            {
                fmt::println("Error when building the triangle vertex shader module");
            }

            pipelineBuilder.SetShaders(skyboxVertexShader, skyboxFragShader);
            pipelineBuilder.SetCullMode(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE);
            pipelineBuilder.EnableDepthTest(VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
            pipelineBuilder.DisableBlending();

            SkyboxPipline = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), skyboxLayout);

            vkDestroyShaderModule(inDevice, skyboxFragShader, nullptr);
            vkDestroyShaderModule(inDevice, skyboxVertexShader, nullptr);
        }

    }

    RenderPipeline::~RenderPipeline()
    {
        OpaquePipeline.reset();
        TransparentPipeline.reset();
        SkyboxPipline.reset();

    }
    
} // namespace MamontEngine