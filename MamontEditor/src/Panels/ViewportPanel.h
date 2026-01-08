#pragma once

#include "EditorPanel.h"


namespace MamontEditor
{
    class ViewportPanel final : public EditorPanel
    {
    public:
        explicit ViewportPanel(const std::string &inName = "Viewport");

        ~ViewportPanel() = default;

        virtual void GuiRender() override;

        virtual bool IsHovered() const override;

    private:
        VkDescriptorSet m_TextureDescriptor{VK_NULL_HANDLE};
        VkSampler       m_TextureSampler{VK_NULL_HANDLE};
    };
} // namespace MamontEditor
