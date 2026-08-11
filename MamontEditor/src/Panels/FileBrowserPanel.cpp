#include "FileBrowserPanel.h"
#include "imgui/plugins/L2DFileDialog.h"
#include "EditorUtils/IconsMaterialsDesign.h"
#include "Core/Log.h"

namespace MamontEditor
{
    std::string fileDialogBuffer;

    FileBrowserPanel::FileBrowserPanel()
        : EditorPanel("File Browser")
    {
        FileDialog::file_dialog_open_type = FileDialog::FileDialogType::SelectFolder;
    }

    FileBrowserPanel::~FileBrowserPanel()
    {
    }

    void FileBrowserPanel::GuiRender()
    {
        FileDialog::ShowFileDialog_s(&FileDialog::file_dialog_open, fileDialogBuffer.data(), FileDialog::file_dialog_open_type, m_Name.c_str());
        
    }

    void FileBrowserPanel::SetCurrentPath(std::string_view inPath)
    {
        fileDialogBuffer = inPath.data();
    }
}