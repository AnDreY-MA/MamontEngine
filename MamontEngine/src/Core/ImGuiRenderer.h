#pragma once 

#include <SDL3/SDL.h>


namespace MamontEngine
{
    struct VkContextDevice;
    struct DeletionQueue;

    class ImGuiRenderer
    {
    public:
        void Init(VkContextDevice &inContextDevice, SDL_Window* inWindow, VkFormat inColorFormat, DeletionQueue &inDeletionQueue);

        void Draw(VkCommandBuffer inCmd, VkImageView inTargetImageView, VkExtent2D inRenderExtent);
    };
}