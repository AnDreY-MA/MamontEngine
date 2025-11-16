#pragma once 

namespace MamontEngine
{
namespace ImmediateContext
{
    void InitImmediateContext(VkQueue graphicQueue, uint32_t queueFamilyIndex);

    void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction);

    void DestroyImmediateContext();
}
}