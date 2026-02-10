#include "Editor.h"

#include <ECS/Components/TransformComponent.h>
#include <Panels/LogPanel.h>
#include "Core/ContextDevice.h"
#include "Core/Engine.h"
#include "ECS/Entity.h"
#include "EditorUtils/EditorUtils.h"
#include "EditorUtils/IconsMaterialsDesign.h"
#include "Fonts/MaterialDesign.h"
#include "ImGuizmo/ImGuizmo.h"
#include "Panels/SceneSettingsPanel.h"
#include "Panels/ViewportPanel.h"
#include "imgui.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

namespace MamontEditor
{
    Editor        *Editor::s_Instance = nullptr;

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
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        io.Fonts->AddFontDefault();
        static const ImWchar icons_ranges[] = {(ImWchar)ICON_PLAY, 0};
        ImFontConfig         icons_config;
        icons_config.MergeMode        = true;
        icons_config.PixelSnapH       = true;
        icons_config.GlyphMinAdvanceX = 13.0f;

        fmt::println("Editor init");

        m_Panels.reserve(5);
        AddPanel<SceneHierarchyPanel>();
        AddPanel<InspectorPanel>();
        AddPanel<StatisticsPanel>();
        AddPanel<LogPanel>();
        AddPanel<SceneSettingsPanel>();
        // AddPanel<ViewportPanel>();

        AddIconFont();
    }

    void Editor::Deactivate()
    {
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
        PROFILE_ZONE("Editor render");

        Begin();

        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            bool panelHovered = false;
            for (const auto &[id, panel] : m_Panels)
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

        End();
    }
    void Editor::DrawMainPanel()
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    GetSceneContext()->Save();
                }

                if (ImGui::MenuItem("Load"))
                {
                    GetSceneContext()->Load();
                }

                ImGui::EndMenu();
            }

            ImGui::SameLine((ImGui::GetWindowContentRegionMax().x * 0.5f) - (1.5f * (ImGui::GetFontSize() + ImGui::GetStyle().ItemSpacing.x)));

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.2f, 0.7f, 0.0f));

            if (ImGui::Button(ICON_PLAY))
            {
                GetSceneContext()->StartScene();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Play");
            }

            ImGui::SameLine();

            if (ImGui::Button(ICON_STOP))
            {
                GetSceneContext()->StopScene();
            }

            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("Stop");
            }

            ImGui::PopStyleColor();

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

    void Editor::AddIconFont()
    {
        ImGuiIO &io = ImGui::GetIO();

        constexpr ImWchar icons_ranges[] = {ICON_MIN_MDI, ICON_MAX_MDI, 0};
        ImFontConfig      icons_config;
        icons_config.MergeMode     = true;
        icons_config.PixelSnapH    = true;
        icons_config.GlyphOffset.y = 1.0f;
        icons_config.OversampleH = icons_config.OversampleV = 1;
        icons_config.GlyphMinAdvanceX                       = 4.0f;
        icons_config.SizePixels                             = 12.0f;

        constexpr float fontSize{16.f};

        io.Fonts->AddFontFromMemoryCompressedTTF(MaterialDesign_compressed_data, MaterialDesign_compressed_size, fontSize, &icons_config, icons_ranges);
    }
} // namespace MamontEditor
