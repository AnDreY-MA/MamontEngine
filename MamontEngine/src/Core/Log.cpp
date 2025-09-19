#include "Log.h"
#include <vadefs.h>

namespace MamontEngine
{
    Log *Log::s_Instance = nullptr;

    Log::Log()
    {
        s_Instance = this;
        s_Instance->Clear();
    }

    Log::~Log()
    {
        s_Instance->Clear();
        s_Instance = nullptr;
    }

    void Log::Clear()
    {
        s_Instance->m_TextBuffer.clear();
        s_Instance->m_LineOffsets.clear();
        s_Instance->m_LineOffsets.push_back(0);
        s_Instance->m_Levels.clear();
    }

    ImGuiTextBuffer &Log::GetTextBuffer()
    {
        return s_Instance->m_TextBuffer;
    }

    const ImVector<int> &Log::GetLineOffsets()
    {
        return s_Instance->m_LineOffsets;
    }

    const ImVector<Log::ELevel> &Log::GetLevels()
    {
        return s_Instance->m_Levels;
    }

}