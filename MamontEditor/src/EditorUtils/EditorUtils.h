#pragma once

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace MamontEditor
{
    std::vector<std::string> ScanFolder(const std::string &folderPath, std::span<const std::string> inExtensions, bool recursive = false);

}