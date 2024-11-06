#pragma once

#include "VkDestriptor.h"
#include "pch.h"

namespace MamontEngine
{
    struct AllocatedImage
    {
        VkImage       Image;
        VkImageView   ImageView;
        VmaAllocation Allocation;
        VkExtent3D    ImageExtent;
        VkFormat      ImageFormat;
    };

    struct AllocatedBuffer
    {
        VkBuffer          Buffer;
        VmaAllocation     Allocation;
        VmaAllocationInfo Info;
    };

    struct Vertex
    {
        glm::vec3 Position;
        float     UV_X;
        glm::vec3 Normal;
        float     UV_Y;
        glm::vec4 Color;
    };

    struct GPUMeshBuffers
    {
        AllocatedBuffer IndexBuffer;
        AllocatedBuffer VertexBuffer;
        VkDeviceAddress VertexBufferAddress;
    };

    struct GPUDrawPushConstants
    {
        glm::mat4       WorldMatrix;
        VkDeviceAddress VertexBuffer;
    };

    struct DeletionQueue
    {
    public:
        void PushFunction(std::function<void()> &&inFunction)
        {
            m_Deletors.push_back(inFunction);
        }

        void Flush()
        {
            for (auto it = m_Deletors.rbegin(); it != m_Deletors.rend(); it++)
            {
                (*it)();
            }

            m_Deletors.clear();
        }

    private:
        std::deque<std::function<void()>> m_Deletors;
    };

    struct FrameData
    {
        VkSemaphore SwapchainSemaphore;
        VkSemaphore RenderSemaphore;

        VkFence RenderFence;

        VkCommandPool   CommandPool;
        VkCommandBuffer MainCommandBuffer;

        DeletionQueue                      Deleteions;
        struct DescriptorAllocatorGrowable FrameDescriptors;
    };

    struct ComputePushConstants
    {
        glm::vec4 Data1;
        glm::vec4 Data2;
        glm::vec4 Data3;
        glm::vec4 Data4;
    };

	enum class EMaterialPass : uint8_t
	{
		MAIN_COLOR, TRANSPARENT, OTHER
	};

	struct MaterialPipeline
	{
        VkPipeline Pipeline;
        VkPipelineLayout Layout;
	};

	struct MaterialInstance
	{
        MaterialPipeline *Pipeline;
        VkDescriptorSet   MaterialSet;
        EMaterialPass      PassType;
	};

    struct BoundsData
    {
        glm::vec3 origin;
        float     sphereRadius;
        glm::vec3 extents;
    };

	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     VkBuffer InIndexBuffer,

                     MaterialInstance* InMaterial,
                     BoundsData InBounds,

                     glm::mat4       InTransform,
                     VkDeviceAddress InVertexBufferAdders)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex)
            , IndexBuffer(InIndexBuffer)
            , Bounds(InBounds)
            , Material(InMaterial)
            , Transform(InTransform)
            , VertexBufferAddress(InVertexBufferAdders)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        VkBuffer IndexBuffer;

		MaterialInstance *Material;
        BoundsData        Bounds;

		glm::mat4 Transform;
        VkDeviceAddress VertexBufferAddress;
	};

    struct DrawContext
    {
        std::vector<RenderObject> OpaqueSurfaces;
        std::vector<RenderObject> TransparentSurfaces;
    };

	struct GLTFMetallic_Roughness
    {
        MaterialPipeline OpaquePipeline;
        MaterialPipeline TransparentPipeline;

        VkDescriptorSetLayout MaterialLayout;

        struct MaterialConstants
        {
            glm::vec4 ColorFactors;
            glm::vec4 Metal_rough_factors;
            glm::vec4 Extra[14];
        };

        struct MaterialResources
        {
            AllocatedImage ColorImage;
            VkSampler      ColorSampler;
            AllocatedImage MetalRoughImage;
            VkSampler      MetalRoughSampler;
            VkBuffer       DataBuffer;
            uint32_t       DataBufferOffset;
        };

        DescriptoeWriter Writer;

        void BuildPipelines(class MEngine *engine);
        void ClearResources(VkDevice device);

        MaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources &resources, DescriptorAllocatorGrowable &descriptorAllocator);
    };


    class IRenderable
    {
    public:
        virtual void Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext) = 0;
    };

    struct Node : public IRenderable
    {
    public:
        Node() = default;
        Node(const glm::mat4 &inLocalTransform, const glm::mat4 &inWorldTransform)
            : m_LocalTransform(inLocalTransform), m_WorldTransform(inWorldTransform)
        {
        }

        void RefreshTransform(const glm::mat4 inParentMatrix)
        {
            m_WorldTransform = inParentMatrix * m_LocalTransform;
        }

        virtual void Draw(const glm::mat4& inTopMatrix, DrawContext& inContext) override
        {
            for (auto& c : m_Children)
            {
                c->Draw(inTopMatrix, inContext);
            }
        }

        std::weak_ptr<Node>                m_Parent;
        std::vector<std::shared_ptr<Node>> m_Children;
        glm::mat4                          m_LocalTransform;
        glm::mat4                          m_WorldTransform;
    };

    struct MeshNode : public Node
    {
    public:
        MeshNode() = default;
        MeshNode(const glm::mat4 &inLocalTransform, const glm::mat4 &inWorldTransform, std::shared_ptr<struct MeshAsset> inMesh)
            : Node(inLocalTransform, inWorldTransform), Mesh(inMesh)
        {
        }
        virtual void Draw(const glm::mat4 &inTopMatrix, DrawContext &inContext) override;

        
        std::shared_ptr<MeshAsset> Mesh;
    };
}