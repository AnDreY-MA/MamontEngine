#pragma once

#include "VkDestriptor.h"
#include "pch.h"


namespace MamontEngine
{
    class MEngine;
} // namespace MamontEngine

namespace MamontEngine
{
    class MEngine;
    struct MeshAsset;

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

	struct RenderObject
	{
        RenderObject() = default;

        RenderObject(uint32_t InIndexCount,
                     uint32_t InFirstIndex,
                     VkBuffer InIndexBuffer,

                     MaterialInstance* InMaterial,

                     glm::mat4       InTransform,
                     VkDeviceAddress InVertexBufferAdders)
            : IndexCount(InIndexCount)
            , FirstIndex(InFirstIndex)
            , IndexBuffer(InIndexBuffer)
            , Material(InMaterial)
            , Transform(InTransform)
            , VertexBufferAddress(InVertexBufferAdders)
        {
        }

        uint32_t IndexCount;
        uint32_t FirstIndex;
        VkBuffer IndexBuffer;

		MaterialInstance *Material;

		glm::mat4 Transform;
        VkDeviceAddress VertexBufferAddress;
	};

    struct DrawContext
    {
        std::vector<RenderObject> OpaqueSurfaces;
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

        VkDescriptor::DescriptoeWriter Writer;

        void BuildPipelines(class MEngine *engine);
        void ClearResources(VkDevice device);

        MaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources &resources, VkDescriptor::DescriptorAllocatorGrowable &descriptorAllocator);
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

        void SetLocalTransform(const glm::mat4& inNew)
        {
            m_LocalTransform = inNew;
        }

        glm::mat4& GetLocalTransform()
        {
            return m_LocalTransform;
        }

        glm::mat4 GetLocalTransform() const
        {
            return m_LocalTransform;
        }

        void AddChild(std::shared_ptr<Node> inChild)
        {
            m_Children.push_back(inChild);
        }

        void SetParent(std::weak_ptr<Node> inParent)
        {
            m_Parent = inParent;
        }

        std::shared_ptr<Node> ParentLock() const
        {
            return m_Parent.lock();
        }

    protected:
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