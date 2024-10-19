#include "VkDestriptor.h"

namespace MamontEngine::VkDescriptor
{

    void DescriptorLayoutBuilder::AddBinding(const uint32_t inBinding, VkDescriptorType inType)
    {
        VkDescriptorSetLayoutBinding newBind{};
        newBind.binding         = inBinding;
        newBind.descriptorCount = 1;
        newBind.descriptorType  = inType;

        m_Bindings.push_back(newBind);
    }

    void DescriptorLayoutBuilder::Clear()
    {
        m_Bindings.clear();
    }

    VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice                         inDevice,
                                                                                     VkShaderStageFlags               inShaderStages,
                                                                                     void                            *pNext,
                                                                                     VkDescriptorSetLayoutCreateFlags flags)
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
    
    void DescriptorAllocator::init_pool(VkDevice inDevice, const uint32_t inMaxSets, std::span<PoolSizeRatio> inPoolRatios)
    {
        std::vector<VkDescriptorPoolSize> poolSizes;
        for (auto ratio : inPoolRatios)
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
        allocInfo.pNext = nullptr;
        allocInfo.descriptorPool = m_Pool;
        allocInfo.descriptorSetCount = 1; 
        allocInfo.pSetLayouts        = &inLayout;

        VkDescriptorSet ds;
        VK_CHECK(vkAllocateDescriptorSets(inDevice, &allocInfo, &ds));

        return ds;
    }

}

