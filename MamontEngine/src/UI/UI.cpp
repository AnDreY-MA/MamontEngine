#include "UI/UI.h"

namespace MamontEngine
{
    inline static int s_ContextID = 0;
    inline static int s_Counter = 0;
    char              MUI::IDBuffer[16];

    void MUI::PushID()
    {
        ++s_ContextID;
        ImGui::PushID(s_ContextID);
        s_Counter = 0;
    }

    void MUI::PopID()
    {
        ImGui::PopID();
        --s_ContextID;
    }

    bool MUI::BeginProperties(ImGuiTableFlags inFlags, const bool inFixedWidth, const float inWidth)
    {
        IDBuffer[0] = '#';
        IDBuffer[1] = '#';
        memset(IDBuffer + 2, 0, 14);
        ++s_Counter;
        const std::string buffer = fmt::format("##{}", s_Counter);
        std::memcpy(&IDBuffer, buffer.data(), 16);

        if (ImGui::BeginTable(IDBuffer, 2, inFlags))
        {
            ImGui::TableSetupColumn("Name");
            if (inFixedWidth)
            {
                ImGui::TableSetupColumn("Property", ImGuiTableColumnFlags_WidthFixed, ImGui::GetWindowWidth() * inWidth);
            }
            else
            {
                ImGui::TableSetupColumn("Property");
            }
            return true;
        }
        return false;
    }

    void MUI::EndProperties()
    {
        ImGui::EndTable();
    }

    void MUI::BeginPropertyGrid(std::string_view inLabel, std::string_view inTooltip, const bool inAlignTextRight)
    {
        PushID();
        PushFrameStyle();

        /*ImGui::TableNextRow();
        if (inAlignTextRight)
        {
            ImGui::SetNextItemWidth(-1.f);
        }
        ImGui::TableNextColumn();*/

        ImGui::PushID(inLabel.data());

        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(inLabel.data());

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-1.0f);

        IDBuffer[0] = '#';
        IDBuffer[1] = '#';
        memset(IDBuffer + 2, 0, 14);
        ++s_Counter;
        const std::string buffer = fmt::format("##{}", s_Counter);
        std::memcpy(&IDBuffer, buffer.data(), 16);
    }
    void MUI::EndPropertyGrid()
    {
        ImGui::PopID();
        PopFrameStyle();
        PopID();
    }
    
    void MUI::PushFrameStyle(const bool on)
    {
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, on ? 1.f : 0.f);
    }

    void MUI::PopFrameStyle()
    {
        ImGui::PopStyleVar();
    }

    bool MUI::DrawVec3Control(std::string_view inLabel, glm::vec3 &inValues, float inResetValue)
    {
        bool changed = false;

        BeginPropertyGrid(inLabel, "tooltip");
        PushFrameStyle(false);

        ImGui::PushMultiItemsWidths(3, ImGui::GetWindowWidth() - 150.f);

        const ImVec2 buttonSize       = {ImGui::GetFrameHeight(), ImGui::GetFrameHeight()};
        const float  frameHeight      = ImGui::GetFrameHeight();
        const ImVec2 actualButtonSize = {frameHeight, frameHeight};

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        // X
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});

            if (ImGui::Button("X", actualButtonSize))
            {
                inValues.x = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &inValues.x, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        // Y
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});

            if (ImGui::Button("Y", actualButtonSize))
            {
                inValues.y = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &inValues.y, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        // Z
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});

            if (ImGui::Button("Z", actualButtonSize))
            {
                inValues.z = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &inValues.z, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
        }

        ImGui::PopStyleVar(); 

        EndPropertyGrid();
        PopFrameStyle();

        return changed;
    }

    bool MUI::DrawQuatControl(std::string_view inLabel, glm::quat &inValues, float inResetValue)
    {
        bool changed = false;

        BeginPropertyGrid(inLabel, "tooltip");
        PushFrameStyle(false);

        ImGui::PushMultiItemsWidths(3, ImGui::GetWindowWidth() - 150.f);

        const ImVec2 buttonSize       = {ImGui::GetFrameHeight(), ImGui::GetFrameHeight()};
        const float  frameHeight      = ImGui::GetFrameHeight();
        const ImVec2 actualButtonSize = {frameHeight, frameHeight};

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

        // X
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.9f, 0.2f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.8f, 0.1f, 0.15f, 1.0f});

            if (ImGui::Button("X", actualButtonSize))
            {
                inValues.x = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##X", &inValues.x, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        // Y
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.3f, 0.8f, 0.3f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.2f, 0.7f, 0.2f, 1.0f});

            if (ImGui::Button("Y", actualButtonSize))
            {
                inValues.y = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Y", &inValues.y, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
        }

        // Z
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{0.2f, 0.35f, 0.9f, 1.0f});
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{0.1f, 0.25f, 0.8f, 1.0f});

            if (ImGui::Button("Z", actualButtonSize))
            {
                inValues.z = inResetValue;
                changed    = true;
            }
            ImGui::PopStyleColor(3);

            ImGui::SameLine();
            if (ImGui::DragFloat("##Z", &inValues.z, 0.1f, 0.0f, 0.0f, "%.2f"))
            {
                changed = true;
            }
            ImGui::PopItemWidth();
        }

        ImGui::PopStyleVar();

        EndPropertyGrid();
        PopFrameStyle();

        return changed;
    }

    bool MUI::InputText(const char* inLabel, std::string *str, size_t bufSize, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *userData)
    {
        PushFrameStyle();
        const bool changed = ImGui::InputText(inLabel, str->data(), bufSize, flags, callback, userData);
        PopFrameStyle();

        return changed;
    }
} // namespace MamontEngine
