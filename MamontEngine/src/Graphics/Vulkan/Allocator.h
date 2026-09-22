#pragma once 

namespace MamontEngine
{
namespace Allocator
{
    VmaAllocator &GetAllocator();

    void Init(const VmaAllocatorCreateInfo &create_info);

    void Destroy();

    void GetHeapBudgets(VmaBudget *outBudgetData);

    uint32_t GetHeapCount();
}
}