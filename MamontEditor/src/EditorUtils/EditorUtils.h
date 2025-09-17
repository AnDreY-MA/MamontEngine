#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace MamontEditor
{
    std::vector<std::string> ScanFolder(const std::string &folderPath, const std::string &extension = "", bool recursive = false);
}