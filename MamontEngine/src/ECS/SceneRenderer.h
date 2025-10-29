#pragma once 

#include "ECS/Components/MeshComponent.h"
#include <Core/Camera.h>

#include <Graphics/Vulkan/Pipelines/RenderPipeline.h>

namespace MamontEngine
{
    struct GPUSceneData
    {
        glm::mat4 View{glm::mat4(0.f)};
        glm::mat4 Proj{glm::mat4(0.f)};
        glm::mat4 Viewproj{glm::mat4(0.f)};
        glm::vec3 LightDirection{glm::vec3(0.4f)};
    };

	class SceneRenderer
	{
    public:
        SceneRenderer(const std::shared_ptr<Camera> &inCamera);

        ~SceneRenderer();

        void Render(VkCommandBuffer  inCmd,
                    VkDescriptorSet  globalDescriptor,
                    const glm::mat4 &inViewProjection,
                    VkPipelineLayout inGlobalLayout = VK_NULL_HANDLE,
                    uint32_t cascadeIndex = 0);
        void RenderShadow(VkCommandBuffer  inCmd,
                          VkDescriptorSet  globalDescriptor,
                          const glm::mat4 &inViewProjection, const PipelineData& inPipelineData,
                          uint32_t         cascadeIndex);

        void RenderPicking(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor, VkPipeline inPipeline, VkPipelineLayout inLayout);

        void Update(const VkExtent2D &inWindowExtent, const std::array<Cascade, CASCADECOUNT> &inCascades);

        void UpdateLight();

        void UpdateCascades(std::array<Cascade, CASCADECOUNT> &outCascades);

        void Clear();

        void RemoveMeshComponent(MeshComponent& inMeshComponent)
        {
            m_MeshComponents.erase(std::remove_if(m_MeshComponents.begin(),
                                                  m_MeshComponents.end(),
                                                  [&inMeshComponent](const MeshComponent &comp)
                                                  {
                                                      return comp.Mesh == inMeshComponent.Mesh;
                                                  }),
                                   m_MeshComponents.end());
        }

        void SubmitMesh(const MeshComponent &inMesh);

        GPUSceneData &GetGPUSceneData()
        {
            return m_SceneData;
        }
        const GPUSceneData &GetGPUSceneData() const
        {
            return m_SceneData;
        }
        const CascadeData& GetCascadeData() const
        {
            return m_CascadeData;
        }

        const Camera* GetCamera() const
        {
            return m_Camera.get();
        }

        const glm::vec3& GetLightPos() const
        {
            return m_LightPosition;
        }

        void ClearDrawContext()
        {
            m_DrawContext.Clear();
        }

	private:
        std::vector<MeshComponent>  m_MeshComponents;
        std::shared_ptr<Camera>     m_Camera;
        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;
        CascadeData  m_CascadeData;

        glm::vec3 m_LightPosition{0.f};

	};

}