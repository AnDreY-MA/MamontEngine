#pragma once 

#include <SDL3/SDL.h>


namespace MamontEngine
{
    struct VkContextDevice;
    struct DeletionQueue;

    class ImGuiRenderer final
    {
    public:
        ImGuiRenderer(const VkContextDevice &inContextDevice, SDL_Window *inWindow, VkFormat inColorFormat, VkDescriptorPool &outPoolResult);

        ~ImGuiRenderer() = default;

        void Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, const VkExtent2D& inRenderExtent);

    };
}