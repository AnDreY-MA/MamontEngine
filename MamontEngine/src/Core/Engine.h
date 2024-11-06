#pragma once

#include "VkDestriptor.h"
#include "Loader.h"
#include "Renderer.h"
#include "Camera.h"
#include "Renderer/SkyPipeline.h"
#include "RenderData.h"
#include "Renderer/MDevice.h"

struct MPipeline
{
    VkPipeline Pipeline;
};

namespace MamontEngine
{
    constexpr unsigned int FRAME_OVERLAP = 2;

    

    struct GPUSceneData
    {
        glm::mat4 View;
        glm::mat4 Proj;
        glm::mat4 Viewproj;
        glm::vec4 AmbientColor;
        glm::vec4 SunlightDirection;
        glm::vec4 SunlightColor;
    };

    struct MEgnineStats
    {
        float frametime;
        int   triangle_count;
        int   drawcall_count;
        float scene_update_time;
        float mesh_draw_time;
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
            return m_MamontDevice.m_Device;
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


    public:
        void InitVulkan();
        void InitSwapchain();
        void InitCommands();
        void InitSyncStructeres();
        void InitDefaultData();

        void CreateSwapchain(const uint32_t inWidth, const uint32_t inHeight);
        void DestroySwapchain();

        void InitDescriptors();

        void InitPipelines();
        void InitBackgrounPipeline();

        void InitImgui();
        //void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&inFunction);
        void DrawImGui(VkCommandBuffer inCmd, VkImageView inTargetImageView);

        void InitTrianglePipeline();
        void InitMeshPipeline();

        void DrawGeometry(VkCommandBuffer inCmd);

         [[nodiscard]] AllocatedBuffer CreateBuffer(size_t inAllocSize, VkBufferUsageFlags inUsage, VmaMemoryUsage inMemoryUsage);
        void            DestroyBuffer(const AllocatedBuffer &inBuffer);

        void ResizeSwapchain();

         [[nodiscard]] AllocatedImage CreateImage(VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false);
        [[nodiscard]] AllocatedImage
             CreateImage(void *inData, VkExtent3D inSize, VkFormat inFormat, VkImageUsageFlags inUsage, const bool inIsMipMapped = false);
        void DestroyImage(const AllocatedImage &inImage);

        void DrawMain(VkCommandBuffer inCmd);

    public:
        bool       m_IsInitialized{false};
        int        m_FrameNumber{0};
        bool       m_StopRendering{false};
        bool       m_IsResizeRequested{false};
        bool       m_IsFreezeRendering{false};

        VkExtent2D m_WindowExtent{1700, 900};

        struct MWindow *Window = nullptr;

        MDevice m_MamontDevice;
        SkyPipeline m_SkyPipeline;

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

        DeletionQueue m_MainDeletionQueue;


        AllocatedImage m_DrawImage;
        AllocatedImage m_DepthImage;

        DescriptorAllocatorGrowable m_GlobalDescriptorAllocator;
        VkDescriptorSet                   m_DrawImageDescriptors;
        VkDescriptorSetLayout             m_DrawImageDescriptorLayout;

        //VkPipelineLayout m_GradientPipelineLayout;
        //MPipeline        m_Pipeline;
        VkPipelineLayout m_TrianglePipelineLayout;
        VkPipeline       m_TrianglePipeline;
        VkPipelineLayout m_MeshPipelineLayout;
        VkPipeline       m_MeshPipeline;
        VkViewport       m_Viewport;

        VkCommandPool   m_ImmCommandPool;

        /*std::vector<ComputeEffect> m_BackgroundEffects;
        int                        m_CurrentBackgroundEffect{1};*/

        GPUMeshBuffers m_Rectangle;

        std::vector<std::shared_ptr<MeshAsset>> m_TestMeshes;

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
        std::unordered_map<std::string, std::shared_ptr<Node>> m_LoadedNodes;
        std::unordered_map<std::string, std::shared_ptr<LoadedGLTF>> loadedScenes;

        void UpdateScene();

        Camera m_MainCamera;

        MEgnineStats stats;
	
    };
}