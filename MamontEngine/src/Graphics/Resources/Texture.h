#pragma once

#include "Asset/Asset.h"

namespace fastgltf
{
    class Asset;
    class Image;
}

namespace MamontEngine
{
    class Texture : public Asset
    {
    public:
        Texture();
        ~Texture();

        VkImage           Image{VK_NULL_HANDLE};
        VkImageView       ImageView{VK_NULL_HANDLE};
        VkSampler         Sampler{VK_NULL_HANDLE};
        VmaAllocation     Allocation{VK_NULL_HANDLE};
        VmaAllocationInfo Info;
        VkExtent3D        ImageExtent{0, 0, 0};
        VkFormat          ImageFormat;
        VkDeviceMemory    DeviceMemory{VK_NULL_HANDLE};

        void Load(void             *inData,
                  const VkExtent3D &inSize,
                  VkFormat          inFormat,
                  VkImageUsageFlags inUsage,
                  const bool        inIsMipMapped = false,
                  VkSampler         inSampler     = VK_NULL_HANDLE);

        void Destroy();

        void Create(const VkExtent3D  &inSize,
                    VkFormat           inFormat,
                    VkImageUsageFlags  inUsage,
                    const bool         inIsMipMapped = false,
                    uint32_t           arrayLayers   = 1,
                    VkImageCreateFlags inCreateFlags = 0, VkSampler inSampler = VK_NULL_HANDLE);

        VkDescriptorImageInfo GetDescriptorInfo() const
        {
            const VkDescriptorImageInfo info {
                .sampler = Sampler, .imageView = ImageView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            return info;
        }

        inline void SetBindId(uint32_t id){ m_Id = id; }
        inline uint32_t GetBindID() const { return m_Id; }

    private:
        uint32_t m_Id{0};
    };

    Texture *LoadCubeMapTexture(const std::string &inFileName, VkFormat inFormat);

    std::shared_ptr<Texture> CreateWhiteTexture();
    Texture* CreateErrorTexture();

    Texture *
    GeneratePrefilteredCube(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &inDrawSkyboxFunc, const Texture &inEnvironmentTexture);
    Texture *
    GenerateIrradianceTexture(VkDeviceAddress vertexAddress, std::function<void(VkCommandBuffer cmd)> &inDrawSkyboxFunc, const Texture &inEnvironmentTexture);

    Texture *GenerateBRDFLUT();

    Texture *CreateTextureFromGltf(const fastgltf::Asset &asset, const fastgltf::Image &image, VkSampler inSampler = VK_NULL_HANDLE);

} // namespace MamontEngine
