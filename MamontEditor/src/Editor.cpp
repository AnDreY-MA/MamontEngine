#include "Editor.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "Core/Engine.h"
#include "ECS/Entity.h"
#include <ECS/Components/TransformComponent.h>
#include <glm/gtc/type_ptr.hpp>

#include <imgui/backends/imgui_impl_vulkan.h>
#include <imgui/backends/imgui_impl_sdl3.h>
#include "ImGuizmo/ImGuizmo.h"

#include "Core/ContextDevice.h"

#include <Panels/ViewportPanel.h>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace MamontEditor
{
    Editor *Editor::s_Instance = nullptr;

    Editor::Editor()
    {
        s_Instance = this;
    }

    Editor *Editor::Get()
    {
        return s_Instance;
    }

    void Editor::Init()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        //                                                      // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; 

        fmt::println("Editor init");

        AddPanel<SceneHierarchyPanel>();
        AddPanel<InspectorPanel>();
        AddPanel<StatisticsPanel>();
        AddPanel<ViewportPanel>();

        //ImGui_ImplVulkanH_CreateOrResizeWindow

    }

    void Editor::Deactivate()
    {
        for (auto &[hash, panel] : m_Panels)
        {
            panel->Deactivate();
        }
        m_Panels.clear();
    }

    void Editor::Begin()
    {
        ImGui_ImplVulkan_NewFrame();
            
        ImGui_ImplSDL3_NewFrame();

        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(ImGui::GetID("MyDockSpace"), ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
        ImGuizmo::BeginFrame();
    }

    void Editor::End()
    {
        const ImGuiIO &io = ImGui::GetIO();   

        ImGui::Render();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    void Editor::ImGuiRender()
    {
        for (auto &[hash, panel] : m_Panels)
        {
            panel->GuiRender();
        }
     
        /*ImGuiWindowFlags          window_flags    = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;


        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;*/

        //// DockSpace
        //ImGuiIO    &io          = ImGui::GetIO();
        //ImGuiStyle &style       = ImGui::GetStyle();
        //float       minWinSizeX = style.WindowMinSize.x;
        //style.WindowMinSize.x   = 370.0f;
        //if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
        //{
        //    ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        //    
        //    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        //}

        //

        //style.WindowMinSize.x = minWinSizeX;
        
        /*static bool               dockspaceOpen             = true;
        static bool               opt_fullscreen_persistant = true;
        bool                      opt_fullscreen            = opt_fullscreen_persistant;*/

        //ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        //ImGui::Begin("Viewport", &dockspaceOpen, window_flags);
        //ImGui::PopStyleVar();
        ////const ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
        ////ImGui::Image(contextDevice.ViewportDescriptor, viewportPanelSize);

        //if (opt_fullscreen)
        //{
        //    ImGuiViewport *viewport = ImGui::GetMainViewport();
        //    ImGui::SetNextWindowPos(viewport->Pos);
        //    ImGui::SetNextWindowSize(viewport->Size);
        //    ImGui::SetNextWindowViewport(viewport->ID);
        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        //    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        //    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        //    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
        //}

        //// Gizmo
        //{
        //    MamontEngine::Entity selectedEntity = GetSelectedEntity();
        //    if (selectedEntity)
        //    {
        //        const auto viewportMinRegion = ImGui::GetWindowContentRegionMin();
        //        const auto viewportMaxRegion = ImGui::GetWindowContentRegionMax();
        //        const auto viewportOffset    = ImGui::GetWindowPos();
        //        m_ViewportBounds[0]          = {viewportMinRegion.x + viewportOffset.x, viewportMinRegion.y + viewportOffset.y};
        //        m_ViewportBounds[1]          = {viewportMaxRegion.x + viewportOffset.x, viewportMaxRegion.y + viewportOffset.y};

        //        ImGuizmo::SetOrthographic(false);
        //        ImGuizmo::SetDrawlist();

        //        // ImGuizmo::SetRect(m_ViewportBounds[0].x, m_ViewportBounds[0].y, m_ViewportBounds[1].x - m_ViewportBounds[0].x, m_ViewportBounds[1].y -
        //        // m_ViewportBounds[0].y);
        //        ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);

        //        const auto *Camera = MamontEngine::MEngine::Get().GetMainCamera();
        //        if (Camera == nullptr)
        //            return;
        //        const glm::mat4 &cameraView       = Camera->GetViewMatrix();
        //        const glm::mat4 &cameraProjection = Camera->GetProjection() * cameraView;


        //        auto     &TransformComp = selectedEntity.GetComponent<MamontEngine::TransformComponent>();
        //        glm::mat4 transform     = TransformComp.GetTransform();

        //        const float snapValue     = 0.5f;
        //        const float snapValues[3] = {snapValue, snapValue, snapValue};
        //        int         gizmoType     = -1;

        //        ImGuizmo::Manipulate(glm::value_ptr(cameraView),
        //                             glm::value_ptr(cameraProjection),
        //                             (ImGuizmo::OPERATION)gizmoType,
        //                             ImGuizmo::LOCAL,
        //                             glm::value_ptr(transform),
        //                             nullptr,
        //                             nullptr);

        //        if (ImGuizmo::IsUsing())
        //        {
        //            glm::vec3 translation, rotation, scale;
        //            // DecomposeTransform(transform, translation, rotation, scale);
        //            ImGuizmo::DecomposeMatrixToComponents(
        //                    glm::value_ptr(cameraView), glm::value_ptr(translation), glm::value_ptr(rotation), glm::value_ptr(scale));
        //            glm::vec3 deltaRotation   = rotation - TransformComp.Rotation;
        //            TransformComp.Translation = translation;
        //            TransformComp.Rotation += deltaRotation;
        //            TransformComp.Scale = scale;
        //        }
        //    }
        //}
        //
        //if (opt_fullscreen)
        //    ImGui::PopStyleVar(2);
        ////ImGui::Image()
        //ImGui::End(); //Viewport
        
    }
}