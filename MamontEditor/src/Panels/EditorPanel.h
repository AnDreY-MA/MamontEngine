#pragma once 

#include <string>
#include <string_view>

namespace MamontEditor
{
    class EditorPanel
    {
    public:
        explicit EditorPanel(std::string_view inName = "EditorPanel");
        virtual ~EditorPanel() = default;

        virtual void Init() {};
        virtual void Deactivate() {};

        virtual void GuiRender() {};

    protected:
        bool OnBegin(int32_t inWindowFlags = 0);
        void OnEnd() const;


	protected:
        std::string m_Name;
        bool        isVisible{true};
	};
}