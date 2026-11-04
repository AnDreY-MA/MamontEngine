#pragma once

#include "Asset/Asset.h"
#include "Core/Event/EventSystem.h"

namespace MamontEngine
{
    class AssetMananger
    {
    public:
        static AssetMananger &Get();

        template <typename AssetClass>
        std::shared_ptr<Asset> LoadAsset(std::string_view inPath);

        static std::vector<std::string> GetExtensionsFromType(EAssetType inType);

        Event<Asset*> OnModelLoaded;
    };
}