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

        void AddMessage(const ELevel inLevel, const std::string& inMessage);

	};
}