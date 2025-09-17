#include "EditorUtils/EditorUtils.h"

namespace MamontEditor
{
    std::vector<std::string> ScanFolder(const std::string &folderPath, const std::string &extension, bool recursive)
    {
        namespace fs = std::filesystem;

        std::vector<std::string> files;

        if (recursive)
        {
            for (const auto &entry : fs::recursive_directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    const std::string filename = entry.path().filename().string();

                    if (extension.empty() || entry.path().extension() == extension)
                    {
                        files.push_back(filename);
                    }
                }
            }
        }
        else
        {
            for (const auto &entry : fs::directory_iterator(folderPath))
            {
                if (entry.is_regular_file())
                {
                    const std::string filename = entry.path().filename().string();

                    if (extension.empty() || entry.path().extension() == extension)
                    {
                        files.push_back(filename);
                    }
                }
            }
        }


        return files;
    }
}
