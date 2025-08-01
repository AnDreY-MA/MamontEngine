#pragma once 

#include "imgui/imgui.h"

namespace MamontEngine
{
    struct AllocatedImage
    {
        VkImage       Image;
        VkImageView   ImageView;
        VmaAllocation Allocation;
        VkExtent3D    ImageExtent;
        VkFormat      ImageFormat;

        ImTextureRef ImTextID;
    };

    struct Image
    {
        AllocatedImage DrawImage;
        AllocatedImage DepthImage;
    };
}