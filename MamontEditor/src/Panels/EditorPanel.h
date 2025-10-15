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

        virtual void Init() {};
        virtual void Deactivate() {};

        virtual void GuiRender() {};

        virtual bool IsHovered() const
        {
            return false;
        }

    protected:
        bool OnBegin(int32_t inWindowFlags = 0);
        void OnEnd() const;


	protected:
        std::string m_Name;
        bool        isVisible{true};
        bool        m_IsHovered;
	};
}