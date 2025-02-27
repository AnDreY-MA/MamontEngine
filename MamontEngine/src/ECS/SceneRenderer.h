#pragma once 
#include "ECS/Components/MeshComponent.h"
#include <Core/Camera.h>

#include <Graphics/Vulkan/Pipelines/RenderPipeline.h>

#include <memory>

namespace MamontEngine
{
    struct GPUSceneData
    {
        glm::mat4 View;
        glm::mat4 Proj;
        glm::mat4 Viewproj;
        glm::vec4 AmbientColor;
        glm::vec4 SunlightDirection;
        glm::vec4 SunlightColor;
    };

	class SceneRenderer
	{
    public:
        SceneRenderer(const std::shared_ptr<Camera>& inCamera, const std::shared_ptr<RenderPipeline> &inRenderPipeline);

        ~SceneRenderer();

        void Update(const VkExtent2D& inWindowExtent);

        void Clear();

        void SubmitMesh(const MeshComponent &inMesh);

        GPUSceneData &GetGPUSceneData()
        {
            return m_SceneData;
        }

	private:
        std::vector<MeshComponent> m_MeshComponents;
        std::shared_ptr<Camera>    m_Camera;
        std::shared_ptr<RenderPipeline> m_RenderPipeline;

        GPUSceneData m_SceneData;
	};

}