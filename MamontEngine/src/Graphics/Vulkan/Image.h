#pragma once 

namespace MamontEngine
{
    struct AllocatedImage
    {
        VkImage       Image{VK_NULL_HANDLE};
        VkImageView   ImageView{VK_NULL_HANDLE};
        VmaAllocation Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;
        VkExtent3D    ImageExtent;
        VkFormat      ImageFormat;
    };

    struct Image
    {
        AllocatedImage DrawImage;
        AllocatedImage DepthImage;
    };
}