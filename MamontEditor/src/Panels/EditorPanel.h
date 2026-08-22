#pragma once 

#include <string>
#include <string_view>

namespace MamontEditor
{
    class EditorPanel
    {
    public:
        explicit EditorPanel(const std::string &inName = "EditorPanel");
        virtual ~EditorPanel() = default;

        virtual void GuiRender() {};

        virtual bool IsHovered() const
        {
            return false;
        }

        inline const std::string &GetName() const { return m_Name; }

        inline bool IsOpened() const { return m_IsOpened; }

    protected:
        bool OnBegin(int32_t inWindowFlags = 0);
        void OnEnd() const;

	protected:
        std::string m_Name;
        bool        m_IsOpened{true};
        bool        m_IsHovered;
	};
}