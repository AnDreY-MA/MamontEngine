

#pragma once


namespace MamontEngine::vkinit
{
    //> init_cmd
    VkCommandPoolCreateInfo     command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
    VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1);
    //< init_cmd

    VkCommandBufferBeginInfo  command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);
    VkCommandBufferSubmitInfo command_buffer_submit_info(VkCommandBuffer cmd);

    VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

    VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

    VkSubmitInfo2
    submit_info(const VkCommandBufferSubmitInfo *cmd, const VkSemaphoreSubmitInfo *signalSemaphoreInfo, const VkSemaphoreSubmitInfo *waitSemaphoreInfo);
    VkPresentInfoKHR present_info(const VkSwapchainKHR *swapchains,
                                  const uint32_t        swapchainCount,
                                  const VkSemaphore    *waitSemaphores,
                                  const uint32_t        waitSemaphoreCount,
                                  const uint32_t       *pImageIndices);

    VkRenderingAttachmentInfo attachment_info(VkImageView view, VkClearValue *clear, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

    VkRenderingAttachmentInfo depth_attachment_info(VkImageView view, VkImageLayout layout /*= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL*/);

    VkRenderingInfo rendering_info(VkExtent2D renderExtent, const VkRenderingAttachmentInfo *colorAttachment, const VkRenderingAttachmentInfo *depthAttachment);

    VkImageSubresourceRange image_subresource_range(VkImageAspectFlags aspectMask);

    VkSemaphoreSubmitInfo           semaphore_submit_info(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore);
    VkDescriptorSetLayoutBinding    descriptorset_layout_binding(VkDescriptorType type, VkShaderStageFlags stageFlags, uint32_t binding);
    VkDescriptorSetLayoutCreateInfo descriptorset_layout_create_info(VkDescriptorSetLayoutBinding *bindings, uint32_t bindingCount);
    VkWriteDescriptorSet            write_descriptor_image(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorImageInfo *imageInfo, uint32_t binding);
    VkWriteDescriptorSet   write_descriptor_buffer(VkDescriptorType type, VkDescriptorSet dstSet, VkDescriptorBufferInfo *bufferInfo, uint32_t binding);
    VkDescriptorBufferInfo buffer_info(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);

    VkImageCreateInfo               image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent, uint32_t arrayLayers = 1);
    VkImageViewCreateInfo           imageview_create_info(
                      VkFormat format, VkImage image, VkImageAspectFlags aspectFlags, const uint32_t inLayerCount = 1, VkImageViewType type = VK_IMAGE_VIEW_TYPE_2D);
    VkImageView                     create_imageview(VkDevice inDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    VkPipelineLayoutCreateInfo      pipeline_layout_create_info();
    VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule, const char *entry = "main");

    VkRenderPassBeginInfo create_render_pass_info(VkRenderPass inRenderPass, VkFramebuffer inFrameBuffer, const VkExtent2D& inSwachainExtent);

    VkVertexInputAttributeDescription
    vertex_input_attribute_description(const uint32_t binding, const uint32_t location, const VkFormat format, const uint32_t offset);

    VkVertexInputBindingDescription vertex_input_binding_description(const uint32_t binding, const uint32_t stride, const VkVertexInputRate inputRate);

    VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info(const std::vector<VkVertexInputBindingDescription>   &vertexBindingDescriptions,
                                                                            const std::vector<VkVertexInputAttributeDescription> &vertexAttributeDescriptions);
} // namespace vkinit
