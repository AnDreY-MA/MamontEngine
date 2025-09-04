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

        void init_pool(VkDevice inDevice, const uint32_t inMaxSets, const std::vector<PoolSizeRatio> &inPoolRatios);
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
            float Ratio{0.0f};
        };

        void Init(VkDevice inDevice, const uint32_t inInitialSets, const std::vector<PoolSizeRatio> &inPoolRatios);
        void ClearPools(VkDevice inDevice);
        void DestroyPools(VkDevice inDevice);

        VkDescriptorSet Allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout, void *pNext = nullptr);

    private:
        VkDescriptorPool GetPool(VkDevice inDevice);
        VkDescriptorPool CreatePool(VkDevice inDevice, const uint32_t inSetCount, const std::vector<PoolSizeRatio> &inPoolRatios);

        std::vector<PoolSizeRatio>    m_Ratios;
        std::vector<VkDescriptorPool> m_FullPools;
        std::vector<VkDescriptorPool> m_ReadyPools;
        uint32_t                      m_SetsPerPool{0};
    };

    struct DescriptorWriter
    {
        std::deque<VkDescriptorImageInfo>  ImageInfos;
        std::deque<VkDescriptorBufferInfo> BufferInfos;
        std::vector<VkWriteDescriptorSet>  Writes;

        void WriteImage(const int inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType);
        void WriteBuffer(const int inBinding, VkBuffer inBuffer, const size_t inSize, const size_t inOffset, VkDescriptorType inType);

        void Clear();
        void UpdateSet(const VkDevice inDevice, VkDescriptorSet& inSet);
    };
}
    

