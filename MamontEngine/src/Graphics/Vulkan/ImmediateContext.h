#pragma once 

namespace MamontEngine
{
namespace ImmediateContext
{
    void InitImmediateContext();
    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction);
}
}