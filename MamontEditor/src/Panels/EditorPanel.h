#pragma once 

#include <string>
#include <string_view>

namespace MamontEditor
{
	class EditorPanel
	{
        explicit EditorPanel(std::string_view inName);

	private:
        std::string m_Name;
	};
}