#pragma once

#include "vk_mem_alloc.h"
#include "VkDestriptor.h"
#include "Loader.h"
#include "Renderer.h"
#include "Camera.h"

struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    constexpr unsigned int FRAME_OVERLAP = 2;

    struct DeletionQueue
    {
    public:
        void PushFunction(std::function < void()>&& inFunction)
        {
            deletors.push_back(inFunction);
        }

        void Flush()
        {
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)();
            }

            deletors.clear();
        }

    private:
        std::deque<std::function<void()>> deletors;

    };

    struct FrameData
    {
        VkSemaphore     SwapchainSemaphore;
        VkSemaphore     RenderSemaphore;

        VkFence         RenderFence;
        
        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;

        DeletionQueue Deleteions;
        DescriptorAllocatorGrowable FrameDescriptors;

    };

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

        GPUMeshBuffers UploadMesh(std::span<uint32_t> inIndices, std::span<Vertex> inVertices);

        VkDevice GetDevice() const
        {
            return m_Device;
        }

        VkDescriptorSetLayout GetGPUSceneData() const
        {
            return m_GPUSceneDataDescriptorLayout;
        }

        AllocatedImage GetDrawImage() const
        {
            return m_DrawImage;
        }
        AllocatedImage GetDepthImage() const
        {
            return m_DepthImage;
        }

        AllocatedImage GetErrorImage() const
        {
            return m_ErrorCheckerboardImage;
        }


    public:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructeres();
        void InitDefaultData();

        void CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight);
        void DestroySwapchain();

        void DrawBackground(VkCommandBuffer inCmd);
        
        void InitDescriptors();

        void InitPipelines();
        void InitBackgrounPipeline();

        void InitImgui();
        
        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction) const;
        
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView) const;

        void DrawMain(VkCommandBuffer inCmd);
        void DrawGeometry(VkCommandBuffer inCmd);

        AllocatedBuffer CreateBuffer(size_t inAllocSize, VkBufferUsageFlags inUsage, VmaMemoryUsage inMemoryUsage) const;
        void            DestroyBuffer(const AllocatedBuffer &inBuffer) const;

        void ResizeSwapchain();

        AllocatedImage CreateImage(VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false) const;
        
        AllocatedImage CreateImage(void* inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false);
        
        void DestroyImage(const AllocatedImage &inImage);

    public:
        bool       m_IsInitialized{false};
        int        m_FrameNumber{0};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        VkExtent2D m_WindowExtent{1700, 900};

        void *m_Window{nullptr};

        VkInstance               m_Instance;
        VkDebugUtilsMessengerEXT m_DebugMessenger;
        VkPhysicalDevice         m_ChosenGPU;
        VkDevice                 m_Device;
        VkSurfaceKHR             m_Surface;

        VkSwapchainKHR           m_Swapchain;
        VkFormat                 m_SwapchainImageFormat;
        std::vector<VkImage>     m_SwapchainImages;
        std::vector<VkImageView> m_SwapchainImageViews;
        VkExtent2D               m_SwapchainExtent;
        VkExtent2D               m_DrawExtent;
        float                    m_RenderScale{1.f};

        FrameData m_Frames[FRAME_OVERLAP];
        /*FrameData &GetCurrentFrame()
        {
            return m_Frames[m_FrameNumber & FRAME_OVERLAP];
        }*/

        VkQueue m_GraphicsQueue;
        uint32_t m_GraphicsQueueFamily;

        DeletionQueue m_MainDeletionQueue;

        VmaAllocator m_Allocator;

        AllocatedImage m_DrawImage;
        AllocatedImage m_DepthImage;

        DescriptorAllocatorGrowable m_GlobalDescriptorAllocator;
        VkDescriptorSet                   m_DrawImageDescriptors;
        VkDescriptorSetLayout             m_DrawImageDescriptorLayout;

        VkPipelineLayout m_GradientPipelineLayout;
        VkPipelineLayout m_TrianglePipelineLayout;
        VkPipeline       m_TrianglePipeline;
        VkPipelineLayout m_MeshPipelineLayout;
        VkPipeline       m_MeshPipeline;
        VkViewport       m_Viewport;

        VkFence         m_ImmFence;
        VkCommandBuffer m_ImmCommandBuffer;
        VkCommandPool   m_ImmCommandPool;

        std::vector<ComputeEffect> m_BackgroundEffects;
        int                        m_CurrentBackgroundEffect{0};

        GPUSceneData m_SceneData;

        VkDescriptorSetLayout m_GPUSceneDataDescriptorLayout;

        AllocatedImage m_WhiteImage;
        AllocatedImage m_BlackImage;
        AllocatedImage m_GreyImage;
        AllocatedImage m_ErrorCheckerboardImage;

        VkSampler m_DefaultSamplerLinear;
        VkSampler m_DefaultSamplerNearest;

        VkDescriptorSetLayout m_SingleImageDescriptorLayout;

        MaterialInstance m_DefualtDataMatInstance;
        GLTFMetallic_Roughness m_MetalRoughMaterial;

        DrawContext m_MainDrawContext;
        std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

        EngineStats stats;

        void UpdateScene();
        void InitRenderable();

        Camera m_MainCamera;
	
    };
}