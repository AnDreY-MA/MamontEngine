#include "Graphics/Resources/Materials/MaterialAllocator.h"
#include "Utils/VkDestriptor.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Utils/Utilities.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Core/Engine.h"
#include "Math/Color.h"
#include "Graphics/Vulkan/ImmediateContext.h"

namespace MamontEngine
{
    namespace MaterialAllocator
    {
        VkDeviceSize                AlignedMaterialSize{0};
        AllocatedBuffer g_Buffer;
        std::array<AllocatedBuffer, FRAME_OVERLAP> stagingBuffers{};

        constexpr size_t MAX_MATERIALS = 2 * 1024;

        std::vector<uint32_t> g_Free;

        uint32_t g_NextIndex{0};

        void Init()
        {
            constexpr std::array<DescriptorAllocatorGrowable::PoolSizeRatio, 1> sizes = {
                    DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6}, };
            
            const VkDevice& device = LogicalDevice::GetDevice();

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(PhysicalDevice::GetDevice(), &properties);
            const size_t minAlignment = static_cast<size_t>(properties.limits.minStorageBufferOffsetAlignment);
            constexpr size_t materialConstSize = sizeof(Material::MaterialConstants);

            AlignedMaterialSize = Utils::AlignUp(materialConstSize, minAlignment);
            const size_t maxMaterialsByLimit = properties.limits.maxStorageBufferRange / AlignedMaterialSize;
            const size_t actualMaxMaterials  = std::min(MAX_MATERIALS, maxMaterialsByLimit);

            g_Buffer.Create(actualMaxMaterials * AlignedMaterialSize,
                            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_2_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                            VMA_MEMORY_USAGE_GPU_ONLY);

            const VkBufferDeviceAddressInfo deviceAddressInfo = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = g_Buffer.Buffer};
            g_Buffer.Address                                  = vkGetBufferDeviceAddress(LogicalDevice::GetDevice(), &deviceAddressInfo);

            for (auto& sBuffer : stagingBuffers)
            {
                sBuffer = CreateStagingBuffer(g_Buffer.Info.size);
            }
        }

        void Destroy()
        {
            const VkDevice &device = LogicalDevice::GetDevice();

            for (auto &sBuffer : stagingBuffers)
            {
                sBuffer.Destroy();
            }

            g_Buffer.Destroy();
        }

        Material *AllocateMaterial(const EMaterialPass pass, const Material::MaterialConstants &data)
        {
            uint32_t  index       = 0;
            if (!g_Free.empty())
            {
                index = g_Free.back();
                g_Free.pop_back();
            }
            else
            {
                index = g_NextIndex++;
            }
            const auto &contextDevice = MEngine::Get().GetContextDevice();

            Material *newMaterial = new Material();
            newMaterial->Constants    = data;
            newMaterial->PassType     = pass;
            newMaterial->Index        = index;

            Update(newMaterial);

            return newMaterial;
        }

        void Free(Material *material)
        {
            g_Free.push_back(material->Index);
            //delete material;
        }

        void Update(const Material *inMaterial)
        {
            if (!inMaterial)
                return;

            const auto       currentFrame = MEngine::Get().GetContextDevice().GetFrame();
            AllocatedBuffer &staging      = stagingBuffers[currentFrame];

            memcpy(staging.Info.pMappedData, &inMaterial->Constants, sizeof(Material::MaterialConstants));

            ImmediateContext::ImmediateSubmit(
                    [&](VkCommandBuffer cmd)
                    {
                        const VkBufferCopy copy{
                            .srcOffset = 0,
                            .dstOffset = inMaterial->Index * AlignedMaterialSize,
                            .size      = sizeof(Material::MaterialConstants)
                        };
                        vkCmdCopyBuffer(cmd, staging.Buffer, g_Buffer.Buffer, 1, &copy);
                    });
        }

        VkDeviceAddress GetBufferAddess()
        {
            return g_Buffer.Address;
        }

    } // namespace MaterialAllocator
} // namespace MamontEngine
