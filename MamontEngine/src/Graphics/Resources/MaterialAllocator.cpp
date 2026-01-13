#include "Graphics/Resources/MaterialAllocator.h"
#include "Graphics/Resources/Material.h"
#include "Utils/VkDestriptor.h"
#include "Graphics/Devices/LogicalDevice.h"
#include "Utils/Utilities.h"
#include "Graphics/Devices/PhysicalDevice.h"
#include "Graphics/Vulkan/Buffers/Buffer.h"
#include "Core/Engine.h"
#include "Math/Color.h"

namespace MamontEngine
{
    namespace MaterialAllocator
    {
        DescriptorAllocatorGrowable DescrtiptorAllocator;
        size_t                      AlignedMaterialSize{0};
        VkDescriptorSet             g_DescriptorSet{VK_NULL_HANDLE};
        AllocatedBuffer g_Buffer;

        constexpr size_t g_MaxMaterials = 2 * 1024;

        std::vector<uint32_t> g_Free;

        uint32_t g_NextIndex{0};

        void Init()
        {
            std::array<DescriptorAllocatorGrowable::PoolSizeRatio, 3> sizes = {
                    DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6},
                    DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6},
                    DescriptorAllocatorGrowable::PoolSizeRatio{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};
            
            const VkDevice& device = LogicalDevice::GetDevice();

            DescrtiptorAllocator.Init(device, 100, sizes);

            VkPhysicalDeviceProperties properties;
            vkGetPhysicalDeviceProperties(PhysicalDevice::GetDevice(), &properties);
            const size_t minAlignment = static_cast<size_t>(properties.limits.minUniformBufferOffsetAlignment);
            constexpr size_t materialConstSize = sizeof(Material::MaterialConstants);

            AlignedMaterialSize = Utils::AlignUp(materialConstSize, minAlignment);
            const size_t maxMaterialsByLimit = properties.limits.maxUniformBufferRange / AlignedMaterialSize;
            const size_t actualMaxMaterials  = std::min(g_MaxMaterials, maxMaterialsByLimit);

            g_Buffer.Create(actualMaxMaterials * AlignedMaterialSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);
        }

        void Destroy()
        {
            const VkDevice &device = LogicalDevice::GetDevice();
            g_DescriptorSet        = VK_NULL_HANDLE;

            DescrtiptorAllocator.ClearPools(device);
            DescrtiptorAllocator.DestroyPools(device);

            g_Buffer.Destroy();

        }

        Material *AllocateMaterial(const EMaterialPass pass, const Material::MaterialResources &resources, const Material::MaterialConstants &data)
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
            newMaterial->Resources    = resources;
            newMaterial->BufferOffset = index * AlignedMaterialSize;
            newMaterial->PassType     = pass;
            newMaterial->Index        = index;

            const VkDevice  &device   = LogicalDevice::GetDevice();

            newMaterial->MaterialSet = DescrtiptorAllocator.Allocate(device, contextDevice.RenderDescriptorLayout);
            DescriptorWriter writer;
            writer.WriteBuffer(0, g_Buffer.Buffer, sizeof(Material::MaterialConstants), newMaterial->BufferOffset, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
            writer.WriteImage(1, resources.ColorTexture->GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(2, resources.MetalRoughTexture->GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(3, resources.NormalTexture->GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(4, resources.EmissiveTexture->GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.WriteImage(5, resources.OcclusionTexture->GetDescriptorInfo(), VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
            writer.UpdateSet(device, newMaterial->MaterialSet);

            g_Buffer.Copy(&newMaterial->Constants, sizeof(Material::MaterialConstants), newMaterial->BufferOffset);

            return newMaterial;
        }

        void Free(Material *material)
        {
            g_Free.push_back(material->Index);
            //delete material;
        }

        void Update(const void *inData, const size_t inOffset)
        {
            g_Buffer.Copy(inData, sizeof(Material::MaterialConstants), inOffset);
        }

    } // namespace MaterialAllocator
} // namespace MamontEngine
