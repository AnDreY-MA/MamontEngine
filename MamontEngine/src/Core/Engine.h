#pragma once

#include "VkDestriptor.h"
#include "Utils/Loader.h"
#include "Camera.h"
#include "FrameData.h"
#include "Window.h"
#include "ContextDevice.h"
#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Swapchain.h"

#include "Graphics/Vulkan/Pipelines/SkyPipeline.h"

#include "Graphics/Renderer.h"
#include "ImGuiRenderer.h"
#include <ECS/SceneRenderer.h>
#include "Graphics/Vulkan/Pipelines/RenderPipeline.h"
#include "ImGuiLayer.h"


struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    class Scene;

    struct EngineStats
    {
        float FrameTime;
        float SceneUpdateTime;
        float MeshDrawTime;
        int   TriangleCount;
        int   DrawCallCount;
    };

	class MEngine
	{
    public:
        void Init();
        void Run();
        void Cleanup();

        static MEngine& Get();

        VkDevice GetDevice() const
        {
            return m_ContextDevice->Device;
        }

        VkContextDevice& GetContextDevice()
        {
            return *m_ContextDevice;
        }

        const EngineStats& GetStats() const
        {
            return stats;
        }

        VkDescriptorSetLayout GetGPUSceneData() const
        {
            return m_GPUSceneDataDescriptorLayout;
        }

        AllocatedImage GetDrawImage() const
        {
            return m_ContextDevice->Image.DrawImage;
        }
        
        AllocatedImage GetWhiteImage() const
        {
            return m_ContextDevice->WhiteImage;
        }

        AllocatedImage GetDepthImage() const
        {
            return m_ContextDevice->Image.DepthImage;
        }

        AllocatedImage GetErrorImage() const
        {
            return m_ContextDevice->ErrorCheckerboardImage;
        }

        VkSampler GetSamplerLinear()
        {
            return m_ContextDevice->DefaultSamplerLinear;
        }

        std::shared_ptr<Scene>& GetScene()
        {
            return m_Scene;
        }

        void PushGuiLayer(ImGuiLayer *inLayer);

        const Camera* GetMainCamera() const
        {
            return m_SceneRenderer->GetCamera();
        }

        uint32_t FindMemoryType(const uint32_t inFilterType, const VkMemoryPropertyFlags inProperties) const;
        VkCommandBuffer BeginSingleTimeCommands(const VkCommandPool &inCommandPool) const;
        void            EndSingleTimeCommands(VkCommandBuffer commandBuffer, const VkCommandPool &cmdPool) const;

        void InsertImageMemoryBarrier(VkCommandBuffer         cmdbuffer,
                                      VkImage                 image,
                                      VkAccessFlags           srcAccessMask,
                                      VkAccessFlags           dstAccessMask,
                                      VkImageLayout           oldImageLayout,
                                      VkImageLayout           newImageLayout,
                                      VkPipelineStageFlags    srcStageMask,
                                      VkPipelineStageFlags    dstStageMask,
                                      VkImageSubresourceRange subresourceRange) const;

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructeres();
        void InitDefaultData();

        void InitDescriptors();
        void CreateRenderPass();
        void InitPipelines();

        void InitImgui();

        void InitFrameBuffers();

        void Draw();
        
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);

        void ResizeSwapchain();

        void InitScene();
        void UpdateScene();

        VkFormat FindDepthFormat() const;
        VkFormat FindSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features) const;

    private:
        bool       m_IsInitialized{false};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        std::shared_ptr<WindowCore> m_Window;

        std::shared_ptr<Renderer> m_Renderer;
        std::shared_ptr<SceneRenderer> m_SceneRenderer;
        ImGuiRenderer m_ImGuiRenderer;
        std::unique_ptr<ImGuiLayer>    m_GuiLayer;

        std::unique_ptr<VkContextDevice> m_ContextDevice;
        MSwapchain                       m_Swapchain;

        VkRenderPass m_RenderPass;
        
        VkExtent2D               m_DrawExtent;
        float                    m_RenderScale{1.f};

        DeletionQueue m_MainDeletionQueue;

        DescriptorAllocator m_GlobalDescriptorAllocator;
        VkDescriptorSet                   m_DrawImageDescriptors;
        VkDescriptorSetLayout             m_DrawImageDescriptorLayout;

        std::unique_ptr<SkyPipeline> m_SkyPipeline;
        std::shared_ptr<RenderPipeline> m_RenderPipeline;

        VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

        std::shared_ptr<Scene> m_Scene;

        EngineStats stats;
        std::shared_ptr<Camera> m_MainCamera;
	
    };
}