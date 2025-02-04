#include "RenderData.h"
#include "Engine.h"
#include "VkPipelines.h"
#include "VkInitializers.h"
#include "Loader.h"
#include "Graphics/Mesh.h"

namespace MamontEngine
{
    const std::string RootDirectories = PROJECT_ROOT_DIR;

    void GLTFMetallic_Roughness::BuildPipelines(MEngine* engine)
    {
        std::string    meshPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.frag.spv";
        VkShaderModule meshFragShader;
        if (!VkPipelines::LoadShaderModule(meshPath.c_str(), engine->GetDevice(), &meshFragShader))
        {
            fmt::println("Error when building the triangle fragment shader module");
        }

        std::string    meshVertexShaderPath = RootDirectories + "/MamontEngine/src/Shaders/mesh.vert.spv";
        VkShaderModule meshVertexShader;
        if (!VkPipelines::LoadShaderModule(meshVertexShaderPath.c_str(), engine->GetDevice(), &meshVertexShader))
        {
            fmt::println("Error when building the triangle vertex shader module");
        }

        VkPushConstantRange matrixRange{};
        matrixRange.offset     = 0;
        matrixRange.size       = sizeof(GPUDrawPushConstants);
        matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        DescriptorLayoutBuilder layoutBuilder;
        layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

        MaterialLayout = layoutBuilder.Build(engine->GetDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

        VkDescriptorSetLayout layouts[] = {engine->GetGPUSceneData(), MaterialLayout};

        VkPipelineLayoutCreateInfo mesh_layout_info = vkinit::pipeline_layout_create_info();
        mesh_layout_info.setLayoutCount             = 2;
        mesh_layout_info.pSetLayouts                = layouts;
        mesh_layout_info.pPushConstantRanges        = &matrixRange;
        mesh_layout_info.pushConstantRangeCount     = 1;

        VkPipelineLayout newLayout;
        VK_CHECK(vkCreatePipelineLayout(engine->GetDevice(), &mesh_layout_info, nullptr, &newLayout));

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

        pipelineBuilder.SetColorAttachmentFormat(engine->GetDrawImage().ImageFormat);
        pipelineBuilder.SetDepthFormat(engine->GetDepthImage().ImageFormat);

        pipelineBuilder.m_PipelineLayout = newLayout;

        OpaquePipeline.Pipeline = pipelineBuilder.BuildPipline(engine->GetDevice());

        pipelineBuilder.EnableBlendingAdditive();

        pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

        TransparentPipeline.Pipeline = pipelineBuilder.BuildPipline(engine->GetDevice());

        vkDestroyShaderModule(engine->GetDevice(), meshFragShader, nullptr);
        vkDestroyShaderModule(engine->GetDevice(), meshVertexShader, nullptr);

    }
    void ClearResources(VkDevice device)
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

    void MeshNode::Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext)
    {
        const glm::mat4 nodeMatrix = inTopMatrix * m_WorldTransform;

        for (auto &s : Mesh->Surfaces)
        {
            RenderObject def(s.Count, s.StartIndex, Mesh->MeshBuffers.IndexBuffer.Buffer, &s.Material->Data, nodeMatrix, Mesh->MeshBuffers.VertexBufferAddress);
            def.Bound = s.Bound;
            if (s.Material->Data.PassType == EMaterialPass::TRANSPARENT)
            {
                inContext.TransparentSurfaces.push_back(def);
            }
            else
            {
                inContext.OpaqueSurfaces.push_back(def);
            }
        }

        Node::Draw(inTopMatrix, inContext);
    }

} // namespace MamontEngine
