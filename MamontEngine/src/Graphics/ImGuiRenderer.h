#pragma once 

#include <SDL3/SDL.h>


namespace MamontEngine
{
    struct VkContextDevice;
    struct DeletionQueue;
    class ImGuiLayer;

    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(const VkContextDevice &inContextDevice, SDL_Window *inWindow, VkFormat inColorFormat, const std::shared_ptr<ImGuiLayer>& inLayer);

        ~ImGuiRenderer();

        void Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, const VkExtent2D& inRenderExtent);

        static ImGuiRenderer &Get();

        static bool IsInitialize();

    private:
        VkDescriptorPool m_DescriptorPool;
        VkFormat         m_ColorFormat;
        std::shared_ptr<ImGuiLayer> m_Layer;

        bool             m_Initialized{false};
    };
}