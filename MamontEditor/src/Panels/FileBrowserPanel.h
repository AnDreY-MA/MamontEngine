#pragma once

#include "EditorPanel.h"

namespace MamontEditor
{
	class FileBrowserPanel : public EditorPanel
	{
    public:
        explicit FileBrowserPanel();
        ~FileBrowserPanel();

        virtual void GuiRender() override;

        void SetCurrentPath(std::string_view inPath);

	private:
	};
}