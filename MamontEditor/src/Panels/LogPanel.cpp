#include "Panels/LogPanel.h"
#include "Core/Log.h"

namespace MamontEditor
{
    LogPanel::LogPanel(const std::string &inName) 
        : EditorPanel(inName)
    {
    }


    void LogPanel::GuiRender()
    {
        if (!OnBegin())
        {
            OnEnd();
            return;
        }

        ImGuiContext &guiContext = *ImGui::GetCurrentContext();

        const bool clear = ImGui::Button("Clear");
        
        ImGui::SameLine();

        if (ImGui::Button("Filter"))
        {
            ImGui::OpenPopup("Filter");
        }
        if (ImGui::BeginPopup("Filter"))
        {
            //ImGui::MenuItem("Info", nullptr, &m_IsShowInfo);
            ImGui::MenuItem("Warning", nullptr, &m_IsShowWarning);
            ImGui::MenuItem("Error", nullptr, &m_IsShowError);

            ImGui::EndPopup();
        }

        ImGui::Separator();

        constexpr ImVec2 sizeLogBar{ImVec2(0, 0)};

        if (ImGui::BeginChild("##Log", sizeLogBar, ImGuiChildFlags_Borders, ImGuiWindowFlags_AlwaysVerticalScrollbar))
        {
            if (clear)
            {
                MamontEngine::Log::Clear();
            }

            const ImGuiDebugLogFlags backup_log_flags = guiContext.DebugLogFlags;
            guiContext.DebugLogFlags &= ~ImGuiDebugLogFlags_EventClipper;

            const auto &textBuffer = MamontEngine::Log::GetTextBuffer();
            const auto &lineOffsets = MamontEngine::Log::GetLineOffsets();
            const auto &levels      = MamontEngine::Log::GetLevels();
            if (textBuffer.size() == 0)
            {
                ImGui::EndChild();
                OnEnd();
                return;
            }
            
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

            const char *buffer   = textBuffer.begin();
            const char *bufferEnd = textBuffer.end();

            ImGuiListClipper clipper;
            clipper.Begin(lineOffsets.Size);
            while (clipper.Step())
            {
                for (int lineNo = clipper.DisplayStart; lineNo < clipper.DisplayEnd; ++lineNo)
                {
                    const int         offset    = lineOffsets[lineNo];
                    if (offset >= textBuffer.size())
                        continue;

                    const char *lineStart = buffer + lineOffsets[lineNo];
                    const char *lineEnd   = (lineNo + 1 < lineOffsets.Size) ? (buffer + lineOffsets[lineNo + 1] - 1) : bufferEnd;

                    if (!m_TextFilter.PassFilter(lineStart, lineEnd))
                        continue;

                    bool pushedColor = false;

                    switch (levels[lineNo])
                    {
                        case MamontEngine::Log::ELevel::Info:
                            if (m_IsShowInfo)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(200, 200, 200, 255));
                                pushedColor = true;
                            }
                            break;
                        case MamontEngine::Log::ELevel::Warning:
                            if (m_IsShowWarning)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 0, 255));
                                pushedColor = true;
                            }
                            break;
                        case MamontEngine::Log::ELevel::Error:
                            if (m_IsShowError)
                            {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 80, 80, 255));
                                pushedColor = true;
                            }
                            break;
                    }
                    if ((levels[lineNo] == MamontEngine::Log::ELevel::Info && !m_IsShowInfo) ||
                        (levels[lineNo] == MamontEngine::Log::ELevel::Warning && !m_IsShowWarning) ||
                        (levels[lineNo] == MamontEngine::Log::ELevel::Error && !m_IsShowError))
                    {
                        continue;
                    }

                    ImGui::TextUnformatted(lineStart, lineEnd);
                    if (pushedColor)
                    {
                        ImGui::PopStyleColor();
                    }
                }
            }

            clipper.End();

            ImGui::PopStyleVar();

            if (AutoScroll && ImGui::GetScrollX() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }

            guiContext.DebugLogFlags = backup_log_flags;
        }
        ImGui::EndChild();

        m_IsHovered = ImGui::IsWindowHovered();


        OnEnd();
    }

    bool LogPanel::IsHovered() const
    {
        return m_IsHovered;
    }

} // namespace MamontEditor