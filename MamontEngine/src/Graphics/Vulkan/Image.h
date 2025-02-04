#pragma once 

namespace MamontEngine
{
    struct AllocatedImage
    {
        VkImage       Image;
        VkImageView   ImageView;
        VmaAllocation Allocation;
        VkExtent3D    ImageExtent;
        VkFormat      ImageFormat;
    };

    struct Image
    {
        AllocatedImage DrawImage;
        AllocatedImage DepthImage;
    };
}