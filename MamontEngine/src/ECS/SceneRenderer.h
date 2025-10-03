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

    struct SceneDataFragment
    {
        float CascadePlits[4];
        glm::mat4 InverseViewMat;
        glm::vec3 LightDirection;
        float     Pad;
        int32_t   ColorCascades;
    };


	class SceneRenderer
	{
    public:
        SceneRenderer(const std::shared_ptr<Camera> &inCamera);

        ~SceneRenderer();

        void Render(VkCommandBuffer inCmd, VkDescriptorSet globalDescriptor, const glm::mat4 &inViewProjection, VkPipelineLayout inGlobalLayout = VK_NULL_HANDLE);

        void Update(const VkExtent2D &inWindowExtent, const std::span<float> inSplitDepths);

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


        const Camera* GetCamera() const
        {
            return m_Camera.get();
        }

        const glm::vec3& GetLightPos() const
        {
            return m_LightPosition;
        }

	private:
        std::vector<MeshComponent>  m_MeshComponents;
        std::shared_ptr<Camera>     m_Camera;
        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;
        SceneDataFragment m_FragmentData;

        glm::vec3 m_LightPosition{0.f};

	};

}