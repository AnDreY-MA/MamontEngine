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
        //glm::vec4 AmbientColor{glm::vec4(0.f)};
        /*glm::vec4 SunlightDirection{glm::vec4(0.f)};
        glm::vec4 SunlightColor{glm::vec4(0.f)};*/
    };

	class SceneRenderer
	{
    public:
        SceneRenderer(const std::shared_ptr<Camera> &inCamera);

        ~SceneRenderer();

        void Render(VkCommandBuffer inCmd, const VkDescriptorSet &globalDescriptor, const GPUSceneData &inSceneData, const VkExtent2D &inDrawExtent);

        void Update(const VkExtent2D& inWindowExtent);

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

	private:
        std::vector<MeshComponent>  m_MeshComponents;
        std::shared_ptr<Camera>     m_Camera;
        DrawContext                m_DrawContext;

        GPUSceneData m_SceneData;

	};

}