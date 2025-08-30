#include "SceneRenderer.h"
#include <glm/gtx/transform.hpp>
#include "Graphics/Model.h"

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
        const glm::mat4& view       = m_Camera->GetViewMatrix();
        const glm::mat4& projection = m_Camera->GetProjection();

        m_Camera->UpdateProjection(inWindowExtent);

        m_SceneData.View     = view;
        m_SceneData.Proj     = projection;
        m_SceneData.Viewproj = projection * view;

        for (auto& mesh : m_MeshComponents)
        {
            if (mesh.Mesh != nullptr)
            {
                mesh.Mesh->Draw(glm::mat4{1.f}, m_DrawContext);
            }
        }
    }

    void SceneRenderer::SubmitMesh(const MeshComponent& inMesh)
    {
        m_MeshComponents.push_back(inMesh);
    }
}