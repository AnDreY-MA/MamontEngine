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

        void Create(const VkExtent3D  &inSize,
                    VkFormat           inFormat,
                    VkImageUsageFlags  inUsage,
                    const bool         inIsMipMapped = false,
                    uint32_t           arrayLayers   = 1,
                    VkImageCreateFlags inCreateFlags = 0);
    };

    Texture LoadCubeMapTexture(const std::string &inFileName, VkFormat inFormat);


    Texture CreateWhiteTexture();


    Texture CreateErrorTexture();

    Texture
    GeneratePrefilteredCube(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &inDrawSkyboxFunc, const Texture &inEnvironmentTexture);

    Texture GenerateBRDFLUT();

} // namespace MamontEngine
