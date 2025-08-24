#pragma once 

#include "imgui/imgui.h"

namespace MamontEngine
{
    struct AllocatedImage
    {
        VkImage       Image{VK_NULL_HANDLE};
        VkImageView   ImageView{VK_NULL_HANDLE};
        VmaAllocation Allocation{VK_NULL_HANDLE};
        VkExtent3D    ImageExtent;
        VkFormat      ImageFormat;

        VkDescriptorSet ImTextID;
    };

    struct Image
    {
        AllocatedImage DrawImage;
        AllocatedImage DepthImage;
    };
}