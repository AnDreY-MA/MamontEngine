#pragma once

#include "VkDestriptor.h"
#include "Utils/Loader.h"
#include "Renderer.h"
#include "Camera.h"
#include "FrameData.h"
#include "Window.h"
#include "ContextDevice.h"
#include "Graphics/Vulkan/GPUBuffer.h"
#include "Graphics/Vulkan/Swapchain.h"
#include "Graphics/MScene.h"

struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct ComputePushConstants
    {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

    struct ComputeEffect
    {
        const char *Name;
        ::VkPipeline  Pipeline;
        VkPipelineLayout Layout;
        
        ComputePushConstants Data;
    };

    struct GPUSceneData
    {
        glm::mat4 View;
        glm::mat4 Proj;
        glm::mat4 Viewproj;
        glm::vec4 AmbientColor;
        glm::vec4 SunlightDirection;
        glm::vec4 SunlightColor;
    };

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

        void Draw();

        static MEngine& Get();

        VkDevice GetDevice() const
        {
            return m_ContextDevice->Device;
        }

        VkDescriptorSetLayout GetGPUSceneData() const
        {
            return m_GPUSceneDataDescriptorLayout;
        }

        AllocatedImage GetDrawImage() const
        {
            return m_Image.DrawImage;
        }
        
        AllocatedImage GetWhiteImage() const
        {
            return m_ContextDevice->WhiteImage;
        }

        AllocatedImage GetDepthImage() const
        {
            return m_Image.DepthImage;
        }

        AllocatedImage GetErrorImage() const
        {
            return m_ContextDevice->ErrorCheckerboardImage;
        }

        VkSampler GetSamplerLinear()
        {
            return m_ContextDevice->DefaultSamplerLinear;
        }

    private:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructeres();
        void InitDefaultData();

        void DrawBackground(VkCommandBuffer inCmd);
        
        void InitDescriptors();

        void InitPipelines();
        void InitBackgrounPipeline();

        void InitImgui();
        
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;
        
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView) const;

        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);

        void ResizeSwapchain();

    private:
        bool       m_IsInitialized{false};
        int        m_FrameNumber{0};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        std::shared_ptr<WindowCore> m_Window;

        std::unique_ptr<VkContextDevice> m_ContextDevice;
        MSwapchain                       m_Swapchain;
        

        VkExtent2D               m_DrawExtent;
        float                    m_RenderScale{1.f};

        std::array<FrameData, FRAME_OVERLAP> m_Frames{};

        FrameData &GetCurrentFrame();

        DeletionQueue m_MainDeletionQueue;

        Image m_Image;

        DescriptorAllocator m_GlobalDescriptorAllocator;
        VkDescriptorSet                   m_DrawImageDescriptors;
        VkDescriptorSetLayout             m_DrawImageDescriptorLayout;

        VkPipelineLayout m_GradientPipelineLayout;
        VkPipelineLayout m_TrianglePipelineLayout;
        VkPipeline       m_TrianglePipeline;
        VkPipelineLayout m_MeshPipelineLayout;
        VkPipeline       m_MeshPipeline;
        VkViewport       m_Viewport;

        std::vector<ComputeEffect> m_BackgroundEffects;
        int                        m_CurrentBackgroundEffect{1};

        GPUSceneData m_SceneData;

        VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

        

        MaterialInstance m_DefualtDataMatInstance;

        DrawContext m_MainDrawContext;
        std::unordered_map<std::string, std::shared_ptr<MScene>> loadedScenes;

        EngineStats stats;

        void UpdateScene();
        void InitRenderable();

        Camera m_MainCamera;
	
    };
}