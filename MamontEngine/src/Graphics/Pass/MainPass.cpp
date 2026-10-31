#include "MainPass.h"

namespace MamontEngine
{
    MainPass::MainPass()
    {
    }

    void MainPass::Render(VkCommandBuffer                             cmd,
                          std::pair<VkDescriptorSet, VkDescriptorSet> globalDescriptor,
                          const DrawContext                          &inDrawContext,
                          const glm::mat4                            &viewproj)
    {
    }
} // namespace MamontEngine
