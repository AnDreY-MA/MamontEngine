#pragma once

namespace MamontEngine::VkDescriptor
{
    struct DescriptorLayoutBuilder
    {
        void AddBinding(const uint32_t inBinding, VkDescriptorType inType);
        void Clear();

        VkDescriptorSetLayout Build(VkDevice inDevice, VkShaderStageFlags inShaderStages, void *pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);

    private:
        std::vector<VkDescriptorSetLayoutBinding> m_Bindings;
    };

    struct DescriptorAllocator
    {
        struct PoolSizeRatio
        {
            VkDescriptorType Type;
            float            Ratio;
        };

        void init_pool(VkDevice inDevice, const uint32_t inMaxSets, std::span<PoolSizeRatio> inPoolRatios);
        void clear_descriptors(VkDevice inDevice);
        void destroy_pool(VkDevice inDevice);

        VkDescriptorSet Allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout);

    private:
        VkDescriptorPool m_Pool;

    };
}
