#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>


namespace MamontEngine
{
    SceneRenderer::SceneRenderer(const std::shared_ptr<Camera> &inCamera, const std::shared_ptr<RenderPipeline> &inRenderPipeline)
        : m_Camera(inCamera), m_RenderPipeline(inRenderPipeline)
    {

    }
    SceneRenderer::~SceneRenderer() 
    {
    }

    void SceneRenderer::Clear()
    {
        m_MeshComponents.clear();

    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent)
    {
        const glm::mat4 view       = m_Camera->GetViewMatrix();
        glm::mat4       projection = glm::perspective(glm::radians(70.f), (float)inWindowExtent.width / (float)inWindowExtent.height, 10000.f, 0.1f);
        projection[1][1] *= -1;

        m_SceneData.View     = view;
        m_SceneData.Proj     = projection;
        m_SceneData.Viewproj = projection * view;

        for (auto& mesh : m_MeshComponents)
        {
            mesh.Mesh->Draw(glm::mat4{1.f}, m_RenderPipeline->MainDrawContext);
        }
    }

    void SceneRenderer::SubmitMesh(const MeshComponent& inMesh)
    {
        m_MeshComponents.push_back(inMesh);
    }
}