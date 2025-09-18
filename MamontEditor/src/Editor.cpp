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

    Editor::~Editor()
    {
        m_Panels.clear();
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
        //AddPanel<ViewportPanel>();

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
        //ImGui::ShowDebugLogWindow();
        //IMGUI_DEBUG_LOG();
        //ImGui::ShowMetricsWindow();
        DrawMainPanel();

        for (auto &[hash, panel] : m_Panels)
        {
            panel->GuiRender();
        }
        
    }

    void Editor::DrawMainPanel()
    {
      if(ImGui::BeginMainMenuBar())
      {
          if (ImGui::BeginMenu("File"))
          {
              if (ImGui::MenuItem("Save"))
              {
                  GetSceneContext()->Save();
              }

              if (ImGui::MenuItem("Load"))
              {
                  GetSceneContext()->Load();
              }

              ImGui::EndMenu();
          }
          ImGui::EndMainMenuBar();
      }
    }
}
