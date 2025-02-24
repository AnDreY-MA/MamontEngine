#include "RenderData.h"
#include "VkPipelines.h"
#include "VkInitializers.h"
#include <glm/gtx/transform.hpp>

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void GLTFMetallic_Roughness::BuildPipelines(VkDevice inDevice, VkDescriptorSetLayout inDescriptorLayout, VkFormat inDrawFormat, VkFormat inDepthFormat)
    {
        const std::string meshPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.frag.spv";
        VkShaderModule meshFragShader;
        if (!VkPipelines::LoadShaderModule(meshPath.c_str(), inDevice, &meshFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        const std::string    meshVertexShaderPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.vert.spv";
        VkShaderModule meshVertexShader;
        if (!VkPipelines::LoadShaderModule(meshVertexShaderPath.c_str(), inDevice, &meshVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }

        const VkPushConstantRange matrixRange{.stageFlags = VK_SHADER_STAGE_VERTEX_BIT, .offset = 0, .size = sizeof(GPUDrawPushConstants)};
        
        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        MaterialLayout = layoutBuilder.Build(inDevice, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        const VkDescriptorSetLayout layouts[] = {inDescriptorLayout, MaterialLayout};

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

        pipelineBuilder.SetColorAttachmentFormat(inDrawFormat);
        pipelineBuilder.SetDepthFormat(inDepthFormat);

        pipelineBuilder.m_PipelineLayout = newLayout;

        OpaquePipeline.Pipeline = pipelineBuilder.BuildPipline(inDevice);

        pipelineBuilder.EnableBlendingAdditive();

        pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

        TransparentPipeline.Pipeline = pipelineBuilder.BuildPipline(inDevice);

        vkDestroyShaderModule(inDevice, meshFragShader, nullptr);
        vkDestroyShaderModule(inDevice, meshVertexShader, nullptr);

    }
    
    void GLTFMetallic_Roughness::ClearResources(VkDevice device)
    {

    }

    MaterialInstance GLTFMetallic_Roughness::WriteMaterial(VkDevice                                         device,
                                                           EMaterialPass                                    pass,
                                                           const GLTFMetallic_Roughness::MaterialResources &resources,
                                                           DescriptorAllocatorGrowable       &descriptorAllocator)
    {
        MaterialInstance matData{};
        matData.PassType = pass;
        if (pass == EMaterialPass::TRANSPARENT)
        {
            matData.Pipeline = &TransparentPipeline;
        }
        else
        {
            matData.Pipeline = &OpaquePipeline;
        }

        matData.MaterialSet = descriptorAllocator.Allocate(device, MaterialLayout);


        Writer.Clear();
        Writer.WriteBuffer(0, resources.DataBuffer, sizeof(MaterialConstants), resources.DataBufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        Writer.WriteImage(
                1, resources.ColorImage.ImageView, resources.ColorSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        Writer.WriteImage(2,
                           resources.MetalRoughImage.ImageView,
                           resources.MetalRoughSampler,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        Writer.UpdateSet(device, matData.MaterialSet);

        return matData;
    }

    

} // namespace MamontEngine
