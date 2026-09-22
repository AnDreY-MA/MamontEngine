#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"

#include "Utils//VkPipelines.h"
#include "Utils//VkDestriptor.h"
#include "Utils//VkInitializers.h"
#include "Core/ContextDevice.h"
#include "ECS/SceneRenderer.h"
//#include "Graphics/Shader/ShaderCompiler.h"
#include "Graphics/Shader/ShaderModule.h"
#include "Core/Log.h"

//#define VMA_IMPLEMENTATION
//#include <vk_mem_alloc.h>

namespace MamontEngine
{
    RenderPipeline::RenderPipeline(VkDevice                              inDevice,
                                   std::span<const VkDescriptorSetLayout> inDescriptorLayouts,
                                   const std::pair<VkFormat, VkFormat>   inImageFormats)
    {
        const std::string meshFragPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/mesh_frag.glsl";
        const std::string meshVertPath = DEFAULT_ASSETS_DIRECTORY + "Shaders/mesh_vert.glsl";

        ShaderModule fragmentModule(inDevice, meshFragPath, EShaderStage::Fragment);
        ShaderModule vertexModule(inDevice, meshVertPath, EShaderStage::Vertex);

        constexpr VkPushConstantRange matrixRange {
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
            .offset = 0, 
            .size = sizeof(GPUDrawPushConstants)
        };

        const VkPipelineLayoutCreateInfo mesh_layout_info =
                vkinit::pipeline_layout_create_info(inDescriptorLayouts.size(), inDescriptorLayouts.data(), &matrixRange, 1);

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &newLayout));

        VkPipelineCacheCreateInfo pipelineCacheInfo{.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
        VkPipelineCache opacPipelineCache{VK_NULL_HANDLE};
        VK_CHECK(vkCreatePipelineCache(inDevice, &pipelineCacheInfo, nullptr, &opacPipelineCache));

        const std::vector<VkVertexInputBindingDescription>  vertexInputBindings{};
        const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes{};
        const VkPipelineVertexInputStateCreateInfo vertexInputInfo =
                vkinit::pipeline_vertex_input_state_create_info(vertexInputBindings, vertexInputAttributes);

        VkPipelines::PipelineBuilder pipelineBuilder;
        pipelineBuilder.SetShaders(vertexModule.GetModule(), fragmentModule.GetModule());
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
        pipelineBuilder.SetCache(opacPipelineCache);

        const VkPipeline opaquePipeline = pipelineBuilder.BuildPipline(inDevice);
        OpaquePipeline                  = std::make_unique<PipelineData>(opaquePipeline, newLayout, opacPipelineCache);

        std::cerr << "OpaquePipeline->Pipeline: " << OpaquePipeline->Pipeline << std::endl;
        std::cerr << "OpaquePipeline->Pipeline, newLayout: " << newLayout << std::endl;


        //Transparent
        {
            VkPipelineCache transPipelineCache{VK_NULL_HANDLE};
            VK_CHECK(vkCreatePipelineCache(inDevice, &pipelineCacheInfo, nullptr, &transPipelineCache));
            VkPipelineLayout transparentLayout;
            VK_CHECK(vkCreatePipelineLayout(inDevice, &mesh_layout_info, nullptr, &transparentLayout));
            pipelineBuilder.SetLayout(transparentLayout);
            pipelineBuilder.EnableBlendingAdditive();
            pipelineBuilder.SetCache(transPipelineCache);

            TransparentPipeline = std::make_shared<PipelineData>(pipelineBuilder.BuildPipline(inDevice), transparentLayout, transPipelineCache);
            std::cerr << "TransparentPipeline->Pipeline: " << TransparentPipeline->Pipeline << std::endl;
            std::cerr << "TransparentPipeline->Pipeline, layout: " << TransparentPipeline->Layout << std::endl;
        }
    }

    RenderPipeline::~RenderPipeline()
    {
        OpaquePipeline.reset();
        TransparentPipeline.reset();
    }
    
} // namespace MamontEngine