#pragma once

namespace MamontEngine::VkUtil
{
    void transition_image(VkCommandBuffer cmd,
                          VkImage         image,
                          VkImageLayout   currentLayout,
                          VkImageLayout   newLayout,
                          uint32_t        mipLevels  = VK_REMAINING_MIP_LEVELS,
                          uint32_t        layerCount = VK_REMAINING_ARRAY_LAYERS);

    void copy_image_to_image(VkCommandBuffer cmd, VkImage source, VkImage destination, const VkExtent2D &srcSize, const VkExtent2D &dstSize);

    void generate_mipmaps(VkCommandBuffer cmd, VkImage image, VkExtent2D imageSize);
}