#include "Editor.h"

#include "Core/Engine.h"
#include "ECS/Entity.h"
#include <ECS/Components/TransformComponent.h>
#include "ImGuizmo/ImGuizmo.h"
#include "Core/ContextDevice.h"
#include <Panels/LogPanel.h>
#include "imgui.h"
#include "EditorUtils/EditorUtils.h"
#include "Panels/SceneSettingsPanel.h"
#include "Panels/ViewportPanel.h"

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
        s_Instance = nullptr;
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
        AddPanel<LogPanel>();
        AddPanel<SceneSettingsPanel>();
        //AddPanel<ViewportPanel>();
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
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            bool panelHovered = false;
            for (const auto& [id, panel] : m_Panels)
            {
                if (panel->IsHovered())
                {
                    panelHovered = true;
                    break;
                }
            }
            if (!panelHovered)
            {
                const ImVec2 mousePos = ImGui::GetMousePos();
                PickObject(mousePos);
            }
        }

        DrawMainPanel();

        for (auto &[hash, panel] : m_Panels)
        {
            panel->GuiRender();
        }
        
    }
    static bool EnableCascade = true;
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

    void Editor::PickObject(const ImVec2 &inMousePostion)
    {
        using namespace MamontEngine;

        const uint64_t pickID = MEngine::Get().TryPickObject({inMousePostion.x, inMousePostion.y});

        auto &Scene = GetSceneContext();
        if (pickID != 0)
        {
            GetPanel<SceneHierarchyPanel>()->SelectEntity(Scene->GetEntity(pickID));
        }
    }
}
