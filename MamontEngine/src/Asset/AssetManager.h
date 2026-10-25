#pragma once

#include "Asset/Asset.h"

namespace MamontEngine
{
    class AssetMananger
    {
    public:
        template <typename AssetClass>
        std::shared_ptr<Asset> LoadAsset(std::string_view inPath);

        static std::vector<std::string> GetExtensionsFromType(EAssetType inType);
    };
}