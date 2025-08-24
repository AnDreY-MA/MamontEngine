#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>


namespace MamontEngine
{
    constexpr float FOV = 70.f;

    SceneRenderer::SceneRenderer(const std::shared_ptr<Camera> &inCamera, DrawContext &inDrawContext) 
        : m_Camera(inCamera), m_DrawContext(inDrawContext)
    {

    }
    SceneRenderer::~SceneRenderer() 
    {
        Clear();
    }

    void SceneRenderer::Clear()
    {
        m_MeshComponents.clear();

    }

    void SceneRenderer::Update(const VkExtent2D &inWindowExtent)
    {
        const glm::mat4 view       = m_Camera->GetViewMatrix();

        m_Camera->UpdateProjection(inWindowExtent);
        
        //const auto AspectRaio{(float)inWindowExtent.width / (float)inWindowExtent.height};
        //glm::mat4  projection = glm::perspective(glm::radians(FOV), AspectRaio, 10000.f, 0.1f);
        //projection[1][1] *= -1;

        m_SceneData.View     = view;
        m_SceneData.Proj     = m_Camera->GetProjection();
        m_SceneData.Viewproj = m_Camera->GetProjection() * view;

        for (auto& mesh : m_MeshComponents)
        {
            mesh.Mesh->Draw(glm::mat4{1.f}, m_DrawContext);
        }
    }

    void SceneRenderer::SubmitMesh(const MeshComponent& inMesh)
    {
        m_MeshComponents.push_back(inMesh);
    }
}