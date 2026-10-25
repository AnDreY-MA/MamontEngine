#include "EditorUtils/EditorUtils.h"

namespace MamontEditor
{
    std::vector<std::string> ScanFolder(const std::string &folderPath, std::span<const std::string> inExtensions, bool recursive)
    {
        namespace fs = std::filesystem;

        std::vector<std::string> files;

        auto proccessFiles = [&files, inExtensions](const std::filesystem::directory_entry &entry) -> void
        {
                if (entry.is_regular_file())
                {
                    const std::string filename = entry.path().filename().string();

                    for (const auto& extension : inExtensions)
                    {
                        if (extension.empty() || entry.path().extension() == extension)
                        {
                            files.push_back(filename);
                        }
                    }
                }
            };

        if (recursive)
        {
            for (const auto &entry : fs::recursive_directory_iterator(folderPath))
            {
                proccessFiles(entry);
            }
        }
        else
        {
            for (const auto &entry : fs::directory_iterator(folderPath))
            {
                proccessFiles(entry);
            }
        }


        return files;
    }
}
