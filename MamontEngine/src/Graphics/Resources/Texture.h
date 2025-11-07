#pragma once

namespace MamontEngine
{
    struct Texture
    {
        VkImage           Image{VK_NULL_HANDLE};
        VkImageView       ImageView{VK_NULL_HANDLE};
        VkSampler         Sampler{VK_NULL_HANDLE};
        VmaAllocation     Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;
        VkExtent3D        ImageExtent{0, 0, 0};
        VkFormat          ImageFormat;
        VkDeviceMemory    DeviceMemory{VK_NULL_HANDLE};

        void Load(void *inData, const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false);

        void Destroy();

    private:
        void Create(const VkExtent3D &inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false, uint32_t arrayLayers = 1);
    };

    Texture CreateWhiteTexture();


    Texture CreateErrorTexture();

} // namespace MamontEngine
