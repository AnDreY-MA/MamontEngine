#include "VkDestriptor.h"

namespace MamontEngine
{
    void DescriptorLayoutBuilder::AddBinding(const uint32_t inBinding, VkDescriptorType inType)
    {
        const VkDescriptorSetLayoutBinding newBind = {
            .binding         = inBinding,
            .descriptorType = inType,
            .descriptorCount = 1,
        };

        m_Bindings.push_back(newBind);
    }

    void DescriptorLayoutBuilder::Clear()
    {
        m_Bindings.clear();
    }

    VkDescriptorSetLayout
    DescriptorLayoutBuilder::Build(VkDevice inDevice, VkShaderStageFlags inShaderStages, void *pNext, VkDescriptorSetLayoutCreateFlags flags)
    {
        for (auto &b : m_Bindings)
        {
            b.stageFlags |= inShaderStages;
        }

        VkDescriptorSetLayoutCreateInfo info = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        info.pNext                           = pNext;

        info.pBindings    = m_Bindings.data();
        info.bindingCount = (uint32_t)m_Bindings.size();
        info.flags        = flags;

        VkDescriptorSetLayout set;
        VK_CHECK(vkCreateDescriptorSetLayout(inDevice, &info, nullptr, &set));

        return set;
    }

    void DescriptorAllocator::init_pool(VkDevice inDevice, const uint32_t inMaxSets, const std::vector<PoolSizeRatio> &inPoolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto &ratio : inPoolRatios)
        {
            poolSizes.push_back(VkDescriptorPoolSize{.type = ratio.Type, .descriptorCount = uint32_t(ratio.Ratio * inMaxSets)});
        }

        VkDescriptorPoolCreateInfo poolInfo = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
        poolInfo.flags                      = 0;
        poolInfo.maxSets                    = inMaxSets;
        poolInfo.poolSizeCount              = (uint32_t)poolSizes.size();
        poolInfo.pPoolSizes                 = poolSizes.data();

        vkCreateDescriptorPool(inDevice, &poolInfo, nullptr, &m_Pool);
    }

    void DescriptorAllocator::clear_descriptors(VkDevice inDevice)
    {
        vkResetDescriptorPool(inDevice, m_Pool, 0);
    }

    void DescriptorAllocator::destroy_pool(VkDevice inDevice)
    {
        vkDestroyDescriptorPool(inDevice, m_Pool, nullptr);
    }

    VkDescriptorSet DescriptorAllocator::Allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout)
    {
        VkDescriptorSetAllocateInfo allocInfo{.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        allocInfo.pNext              = nullptr;
        allocInfo.descriptorPool     = m_Pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &inLayout;

        VkDescriptorSet ds;
        VK_CHECK(vkAllocateDescriptorSets(inDevice, &allocInfo, &ds));

        return ds;
    }


    ///// DescriptorAllocatorGrowable

    void DescriptorAllocatorGrowable::Init(VkDevice inDevice, const uint32_t inInitialSets, const std::vector<PoolSizeRatio>& inPoolRatios)
    {
        const uint32_t initSize = inInitialSets != 0 ? inInitialSets : 1;
        m_Ratios.assign(inPoolRatios.begin(), inPoolRatios.end());

        VkDescriptorPool newPool = CreatePool(inDevice, initSize, inPoolRatios);
        m_SetsPerPool            = initSize * 1.5;
        m_ReadyPools.push_back(newPool);
    }

    void DescriptorAllocatorGrowable::ClearPools(VkDevice inDevice)
    {
        for (auto p : m_ReadyPools)
        {
            vkResetDescriptorPool(inDevice, p, 0);
        }

        for (auto p : m_FullPools)
        {
            vkResetDescriptorPool(inDevice, p, 0);
            m_ReadyPools.push_back(p);
        }

        m_FullPools.clear();
    }

    void DescriptorAllocatorGrowable::DestroyPools(VkDevice inDevice)
    {
        for (const auto& p : m_ReadyPools)
        {
            vkDestroyDescriptorPool(inDevice, p, nullptr);
        }
        m_ReadyPools.clear();

        for (const auto &p : m_FullPools)
        {
            vkDestroyDescriptorPool(inDevice, p, nullptr);
        }
        m_FullPools.clear();
    }

    VkDescriptorSet DescriptorAllocatorGrowable::Allocate(VkDevice inDevice, VkDescriptorSetLayout inLayout, void *pNext)
    {
        VkDescriptorPool poolToUse = GetPool(inDevice);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.pNext                       = pNext;
        allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool              = poolToUse;
        allocInfo.descriptorSetCount          = 1;
        allocInfo.pSetLayouts                 = &inLayout;

        VkDescriptorSet newDescriptors;
        const VkResult        result = vkAllocateDescriptorSets(inDevice, &allocInfo, &newDescriptors);

        // allocation failed. Try again
        if (result == VK_ERROR_OUT_OF_POOL_MEMORY || result == VK_ERROR_FRAGMENTED_POOL)
        {
            m_FullPools.push_back(poolToUse);

            poolToUse                = GetPool(inDevice);
            allocInfo.descriptorPool = poolToUse;

            VK_CHECK(vkAllocateDescriptorSets(inDevice, &allocInfo, &newDescriptors));
        }

        m_FullPools.push_back(poolToUse);
        return newDescriptors;
    }

    VkDescriptorPool DescriptorAllocatorGrowable::GetPool(VkDevice inDevice)
    {
        VkDescriptorPool newPool;
        if (m_ReadyPools.size() != 0)
        {
            newPool = m_ReadyPools.back();
            m_ReadyPools.pop_back();
        }
        else
        {
            newPool = CreatePool(inDevice, m_SetsPerPool, m_Ratios);

            m_SetsPerPool = m_SetsPerPool * 1.5;
            if (m_SetsPerPool > 4092)
            {
                m_SetsPerPool = 4092;
            }
        }

        return newPool;
    }

    VkDescriptorPool DescriptorAllocatorGrowable::CreatePool(VkDevice inDevice, const uint32_t inSetCount, const std::vector<PoolSizeRatio> &inPoolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (const auto &ratio : inPoolRatios)
        {
            poolSizes.push_back(VkDescriptorPoolSize{.type = ratio.type, .descriptorCount = uint32_t(ratio.Ratio * inSetCount)});
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags                      = 0;
        poolInfo.maxSets                    = inSetCount;
        poolInfo.poolSizeCount              = (uint32_t)poolSizes.size();
        poolInfo.pPoolSizes                 = poolSizes.data();

        VkDescriptorPool newPool;
        vkCreateDescriptorPool(inDevice, &poolInfo, nullptr, &newPool);
        return newPool;
    }

    //// DescriptoeWriter

    void DescriptorWriter::WriteImage(const int inBinding, VkImageView inImage, VkSampler inSampler, VkImageLayout inLayout, VkDescriptorType inType)
    {
        const VkDescriptorImageInfo &info = ImageInfos.emplace_back(VkDescriptorImageInfo{.sampler = inSampler, .imageView = inImage, .imageLayout = inLayout});

        VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstBinding      = inBinding;
        write.dstSet          = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType  = inType;
        write.pImageInfo      = &info;

        Writes.push_back(write);
    }
    void DescriptorWriter::WriteBuffer(const int inBinding, VkBuffer& inBuffer, const size_t inSize, const size_t inOffset, VkDescriptorType inType)
    {
        const VkDescriptorBufferInfo &info = BufferInfos.emplace_back(VkDescriptorBufferInfo{.buffer = inBuffer, .offset = inOffset, .range = inSize});

        VkWriteDescriptorSet write = {.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};

        write.dstBinding      = inBinding;
        write.dstSet          = VK_NULL_HANDLE;
        write.descriptorCount = 1;
        write.descriptorType  = inType;
        write.pBufferInfo     = &info;

        Writes.push_back(write);
    }

    void DescriptorWriter::Clear()
    {
        ImageInfos.clear();
        Writes.clear();
        BufferInfos.clear();
    }
    void DescriptorWriter::UpdateSet(const VkDevice inDevice, VkDescriptorSet& inSet)
    {
        for (VkWriteDescriptorSet &write : Writes)
        {
            write.dstSet = inSet;
        }

        vkUpdateDescriptorSets(inDevice, (uint32_t)Writes.size(), Writes.data(), 0, nullptr);
    }

}