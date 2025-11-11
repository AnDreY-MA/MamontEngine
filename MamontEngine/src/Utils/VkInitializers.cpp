#include "VkInitializers.h"

//> init_cmd
VkCommandPoolCreateInfo MamontEngine::vkinit::command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags /*= 0*/)
{
    const VkCommandPoolCreateInfo info = {
        .sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = flags,
        .queueFamilyIndex = queueFamilyIndex
    };

    return info;
}


VkCommandBufferAllocateInfo MamontEngine::vkinit::command_buffer_allocate_info(VkCommandPool pool, uint32_t count /*= 1*/)
{
    VkCommandBufferAllocateInfo info = {};
    info.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    info.pNext                       = nullptr;

    info.commandPool        = pool;
    info.commandBufferCount = count;
    info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    return info;
}
//< init_cmd
//
//> init_cmd_draw
VkCommandBufferBeginInfo MamontEngine::vkinit::command_buffer_begin_info(VkCommandBufferUsageFlags flags /*= 0*/)
{
    VkCommandBufferBeginInfo info = {};
    info.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    info.pNext                    = nullptr;

    info.pInheritanceInfo = nullptr;
    info.flags            = flags;
    return info;
}
//< init_cmd_draw

//> init_sync
VkFenceCreateInfo MamontEngine::vkinit::fence_create_info(VkFenceCreateFlags flags /*= 0*/)
{
    VkFenceCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.pNext             = nullptr;

    info.flags = flags;

    return info;
}

VkSemaphoreCreateInfo MamontEngine::vkinit::semaphore_create_info(VkSemaphoreCreateFlags flags /*= 0*/)
{
    VkSemaphoreCreateInfo info = {};
    info.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.pNext                 = nullptr;
    info.flags                 = flags;
    return info;
}
//< init_sync

//> init_submit
VkSemaphoreSubmitInfo MamontEngine::vkinit::semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
{
    VkSemaphoreSubmitInfo submitInfo{};
    submitInfo.sType       = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
    submitInfo.pNext       = nullptr;
    submitInfo.semaphore   = semaphore;
    submitInfo.stageMask   = stageMask;
    submitInfo.deviceIndex = 0;
    submitInfo.value       = 1;

    return submitInfo;
}

VkCommandBufferSubmitInfo MamontEngine::vkinit::command_buffer_submit_info(VkCommandBuffer cmd)
{
    VkCommandBufferSubmitInfo info{};
    info.sType         = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
    info.pNext         = nullptr;
    info.commandBuffer = cmd;
    info.deviceMask    = 0;

    return info;
}

VkSubmitInfo2 MamontEngine::vkinit::submit_info(const VkCommandBufferSubmitInfo *cmd, const VkSemaphoreSubmitInfo *signalSemaphoreInfo, const VkSemaphoreSubmitInfo *waitSemaphoreInfo)
{
    VkSubmitInfo2 info = {};
    info.sType         = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext         = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos    = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos    = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos    = cmd;

    return info;
}
//< init_submit

VkPresentInfoKHR MamontEngine::vkinit::present_info(const VkSwapchainKHR *swapchains,
                                                    const uint32_t        swapchainCount,
                                                    const VkSemaphore    *waitSemaphores,
                                                    const uint32_t        waitSemaphoreCount,
                                                    const uint32_t       *pImageIndices)
{
    const auto info = VkPresentInfoKHR {
        .sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext            = 0,
        .waitSemaphoreCount = waitSemaphoreCount,
        .pWaitSemaphores    = waitSemaphores,
        .swapchainCount     = swapchainCount,
        .pSwapchains        = swapchains,
        .pImageIndices      = pImageIndices,
        .pResults = nullptr
    };

    return info;
}

//> color_info
VkRenderingAttachmentInfo
MamontEngine::vkinit::attachment_info(VkImageView view, VkClearValue *clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/)
{
    VkRenderingAttachmentInfo colorAttachment{};
    colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    colorAttachment.pNext = nullptr;

    colorAttachment.imageView   = view;
    colorAttachment.imageLayout = layout;
    colorAttachment.loadOp      = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp     = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.storeOp            = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.resolveMode        = VK_RESOLVE_MODE_NONE;
    colorAttachment.resolveImageView   = VK_NULL_HANDLE;
    colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (clear)
    {
        colorAttachment.clearValue = *clear;
    }

    return colorAttachment;
}
//< color_info
//> depth_info
VkRenderingAttachmentInfo MamontEngine::vkinit::depth_attachment_info(VkImageView                     view,
                                                                      VkAttachmentLoadOp loadOp,
                                                                      float clearDepth, VkImageLayout layout /*= VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL*/)
{
    VkRenderingAttachmentInfo depthAttachment{};
    depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    depthAttachment.pNext = nullptr;

    depthAttachment.imageView                     = view;
    depthAttachment.imageLayout                   = layout;
    depthAttachment.loadOp                        = loadOp;
    depthAttachment.loadOp                        = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp                       = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachment.clearValue.depthStencil.depth   = clearDepth;
    depthAttachment.clearValue.depthStencil.stencil = 0.f;

    return depthAttachment;
}
//< depth_info
//> render_info
VkRenderingInfo MamontEngine::vkinit::rendering_info(VkExtent2D                       renderExtent,
                                                     const VkRenderingAttachmentInfo *colorAttachment,
                                                     const VkRenderingAttachmentInfo *depthAttachment)
{
    VkRenderingInfo renderInfo{};
    renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderInfo.pNext = nullptr;

    renderInfo.renderArea           = VkRect2D{VkOffset2D{0, 0}, renderExtent};
    renderInfo.layerCount           = 1;
    renderInfo.colorAttachmentCount = colorAttachment != nullptr? 1 : 0;
    renderInfo.pColorAttachments    = colorAttachment;
    renderInfo.pDepthAttachment     = depthAttachment;
    renderInfo.pStencilAttachment   = nullptr;

    return renderInfo;
}
//< render_info
//> subresource
VkImageSubresourceRange MamontEngine::vkinit::image_subresource_range(VkImageAspectFlags aspectMask)
{
    VkImageSubresourceRange subImage{};
    subImage.aspectMask     = aspectMask;
    subImage.baseMipLevel   = 0;
    subImage.levelCount     = VK_REMAINING_MIP_LEVELS;
    subImage.baseArrayLayer = 0;
    subImage.layerCount     = VK_REMAINING_ARRAY_LAYERS;

    return subImage;
}
//< subresource


VkDescriptorSetLayoutBinding MamontEngine::vkinit::descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding)
{
    VkDescriptorSetLayoutBinding setbind = {};
    setbind.binding                      = binding;
    setbind.descriptorCount              = 1;
    setbind.descriptorType               = type;
    setbind.pImmutableSamplers           = nullptr;
    setbind.stageFlags                   = stageFlags;

    return setbind;
}

VkDescriptorSetLayoutCreateInfo MamontEngine::vkinit::descriptorset_layout_create_info(VkDescriptorSetLayoutBinding *bindings, uint32_t bindingCount)
{
    VkDescriptorSetLayoutCreateInfo info = {};
    info.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    info.pNext                           = nullptr;

    info.pBindings    = bindings;
    info.bindingCount = bindingCount;
    info.flags        = 0;

    return info;
}

VkWriteDescriptorSet
MamontEngine::vkinit::write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding)
{
    VkWriteDescriptorSet write = {};
    write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext                = nullptr;

    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pImageInfo      = imageInfo;

    return write;
}

VkWriteDescriptorSet
MamontEngine::vkinit::write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding)
{
    VkWriteDescriptorSet write = {};
    write.sType                = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.pNext                = nullptr;

    write.dstBinding      = binding;
    write.dstSet          = dstSet;
    write.descriptorCount = 1;
    write.descriptorType  = type;
    write.pBufferInfo     = bufferInfo;

    return write;
}

VkDescriptorBufferInfo MamontEngine::vkinit::buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
{
    VkDescriptorBufferInfo binfo{};
    binfo.buffer = buffer;
    binfo.offset = offset;
    binfo.range  = range;
    return binfo;
}

//> image_set
VkImageCreateInfo MamontEngine::vkinit::image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t arrayLayers)
{
    VkImageCreateInfo info = {};
    info.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    info.pNext             = nullptr;

    info.imageType = VK_IMAGE_TYPE_2D;

    info.format = format;
    info.extent = extent;

    info.mipLevels   = 1;
    info.arrayLayers = arrayLayers;

    // for MSAA. we will not be using it by default, so default it to 1 sample per pixel.
    info.samples = VK_SAMPLE_COUNT_1_BIT;

    // optimal tiling, which means the image is stored on the best gpu format
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage  = usageFlags;

    return info;
}

VkImageViewCreateInfo MamontEngine::vkinit::imageviewCreateInfo(VkFormat           format,
                                                                                      VkImage            image,
                                                                                      VkImageAspectFlags aspectFlags,
                                                                                      uint32_t           mipLevels,
                                                                                      uint32_t           layerCount,
                                                                                      VkImageViewType    viewType)
{
    VkImageViewCreateInfo info{};
    info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    info.image                           = image;
    info.viewType                        = viewType;
    info.format                          = format;
    info.subresourceRange.baseMipLevel   = 0;
    info.subresourceRange.levelCount     = mipLevels;
    info.subresourceRange.baseArrayLayer = 0;
    info.subresourceRange.layerCount     = layerCount;
    info.subresourceRange.aspectMask     = aspectFlags;
    return info;
}

VkImageView MamontEngine::vkinit::create_imageview(VkDevice inDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = imageviewCreateInfo(format, image, aspectFlags);
    VkImageView           imageView;
    if (vkCreateImageView(inDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create image view");
    }
    return imageView;
} 
//< image_set
VkPipelineLayoutCreateInfo
MamontEngine::vkinit::pipeline_layout_create_info(const uint32_t inLayoutCount, const VkDescriptorSetLayout *inLaouts, const VkPushConstantRange* inPushConstRange, const uint32_t inPushRangeCount)
{
    VkPipelineLayoutCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    info.pNext = nullptr;

    // empty defaults
    info.flags                  = 0;
    info.setLayoutCount         = inLayoutCount;
    info.pSetLayouts            = inLaouts;
    info.pushConstantRangeCount = inPushRangeCount;
    info.pPushConstantRanges    = inPushConstRange;
    return info;
}

VkPipelineShaderStageCreateInfo
MamontEngine::vkinit::pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char *entry)
{
    VkPipelineShaderStageCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.pNext = nullptr;

    // shader stage
    info.stage = stage;
    // module containing the code for this shader stage
    info.module = shaderModule;
    // the entry point of the shader
    info.pName = entry;
    return info;
}

VkRenderPassBeginInfo MamontEngine::vkinit::create_render_pass_info(VkRenderPass inRenderPass, VkFramebuffer inFrameBuffer, const VkExtent2D &inSwachainExtent)
{
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass        = inRenderPass;
    renderPassInfo.framebuffer       = inFrameBuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = inSwachainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color        = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    return renderPassInfo;
}

VkVertexInputAttributeDescription
MamontEngine::vkinit::vertex_input_attribute_description(const uint32_t binding, const uint32_t location, const VkFormat format, const uint32_t offset)
{
    const VkVertexInputAttributeDescription vInputAttribDescription{
        .location = location,
        .binding  = binding,
        .format   = format,
        .offset   = offset
    };
    return vInputAttribDescription;
}

VkVertexInputBindingDescription
MamontEngine::vkinit::vertex_input_binding_description(const uint32_t binding, const uint32_t stride, const VkVertexInputRate inputRate)
{
    const VkVertexInputBindingDescription vertexInputBindDescription{
        .binding   = binding,
        .stride    = stride,
        .inputRate = inputRate
    };
    return vertexInputBindDescription;
}

VkPipelineVertexInputStateCreateInfo
MamontEngine::vkinit::pipeline_vertex_input_state_create_info(const std::vector<VkVertexInputBindingDescription>   &vertexBindingDescriptions,
                                                              const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions)
{
    VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo{};
    pipelineVertexInputStateCreateInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount   = static_cast<uint32_t>(vertexBindingDescriptions.size());
    pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions      = vertexBindingDescriptions.data();
    pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexAttributeDescriptions.size());
    pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions    = vertexAttributeDescriptions.data();
    return pipelineVertexInputStateCreateInfo;
}
