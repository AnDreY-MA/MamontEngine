#pragma once

namespace MamontEngine
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

    struct DescriptorAllocatorGrowable
    {
    public:
        struct PoolSizeRatio
        {
            VkDescriptorType type;
            float            Ratio;
        };

        void Init(VkDevice inDevice, const uint32_t inInitialSets, std::span<PoolSizeRatio> inPoolRatios);
        void ClearPools(VkDevice inDevice);
        void DestroyPools(VkDevice inDevice);

        VkDescriptorSet Allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout, void *pNext = nullptr);

    private:
        VkDescriptorPool GetPool(VkDevice inDevice);
        VkDescriptorPool CreatePool(VkDevice inDevice, const uint32_t inSetCount, std::span<PoolSizeRatio> inPoolRatios);

        std::vector<PoolSizeRatio>    m_Ratios;
        std::vector<VkDescriptorPool> m_FullPools;
        std::vector<VkDescriptorPool> m_ReadyPools;
        uint32_t                      m_SetsPerPool;
    };

    struct DescriptorWriter
    {
        std::deque<VkDescriptorImageInfo>  m_ImageInfos;
        std::deque<VkDescriptorBufferInfo> m_BufferInfos;
        std::vector<VkWriteDescriptorSet>  m_Writes;

        void WriteImage(const int inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType);
        void WriteBuffer(const int inBinding, VkBuffer inBuffer, const size_t inSize, const size_t inOffset, VkDescriptorType inType);

        void Clear();
        void UpdateSet(VkDevice inDevice, VkDescriptorSet inSet);
    };
}
    

