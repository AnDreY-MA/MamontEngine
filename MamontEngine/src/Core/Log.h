#pragma once
#include <cstdarg>
namespace MamontEngine
{
	class Log : NonMovable
	{
    public:
        explicit Log();
        ~Log();

        enum class ELevel : uint8_t
        {
            Info = 0, 
            Warning,
            Error
        };

        /*template <typename... Args>
        static void Info(std::format_string<Args...> fmt, Args &&...args)
        {
            std::string message = std::format(fmt, std::forward<Args>(args)...);
            message += '\n';

            int old_size = s_Instance->m_TextBuffer.size();
            s_Instance->m_TextBuffer.append(message.c_str());

            for (int new_size = s_Instance->m_TextBuffer.size(); old_size < new_size; old_size++)
            {
                if (s_Instance->m_TextBuffer[old_size] == '\n')
                {
                    s_Instance->m_LineOffsets.push_back(static_cast<int>(old_size + 1));
                }
            }
        }*/

        template <typename... Args>
        static void Info(std::format_string<Args...> fmt, Args &&...args)
        {
            s_Instance->AddMessage(ELevel::Info, std::format(fmt, std::forward<Args>(args)...));
        }

        template <typename... Args>
        static void Warn(std::format_string<Args...> fmt, Args &&...args)
        {
            s_Instance->AddMessage(ELevel::Warning, std::format(fmt, std::forward<Args>(args)...));
        }

        template <typename... Args>
        static void Error(std::format_string<Args...> fmt, Args &&...args)
        {
            s_Instance->AddMessage(ELevel::Error, std::format(fmt, std::forward<Args>(args)...));
        }

        static ImGuiTextBuffer& GetTextBuffer();

        static const ImVector<int> &GetLineOffsets();

        static const ImVector<ELevel> &GetLevels();

        static void Clear();

	private:
        ImGuiTextBuffer m_TextBuffer;
        ImVector<int>   m_LineOffsets; 
        ImVector<ELevel> m_Levels;

        static Log *s_Instance;

        void AddMessage(const ELevel inLevel, const std::string& inMessage)
        {
            int         old_size    = m_TextBuffer.size();
            std::string withNewline = inMessage + '\n';
            m_TextBuffer.append(withNewline.c_str());

            for (int i = old_size; i < m_TextBuffer.size(); i++)
            {
                if (m_TextBuffer[i] == '\n')
                {
                    m_LineOffsets.push_back(i + 1);
                    m_Levels.push_back(inLevel);
                }
            }
        }

	};
}