#include "Graphics/Resources/AssetManager.h"
#include "Graphics/Resources/Asset.h"
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <fstream>
#include <filesystem>
#include "Core/Log.h"

namespace MamontEngine
{
namespace AssetMananger
{
    static std::shared_ptr<Asset> DeserializeAsset(std::string_view inPath)
    {
        std::shared_ptr<Asset> asset = nullptr;

        if (!std::filesystem::exists(std::filesystem::path(inPath.data())))
        {
            Log::Warn("Asset Manager: asset path {} not exists", inPath.data());
            return nullptr;
        }

        std::ifstream              file(inPath.data(), std::ios::binary);
        cereal::BinaryInputArchive archive(file);
        archive(*asset);

        return asset;
    }


    template <typename AssetClass>
    std::shared_ptr<Asset> LoadAsset(std::string_view inPath)
    {
        std::shared_ptr<Asset> asset = DeserializeAsset(inPath);

        return std::dynamic_pointer_cast<AssetClass>(asset);
    }
}
} // namespace MamontEngine
