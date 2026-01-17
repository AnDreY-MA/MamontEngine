#pragma once

namespace MamontEngine
{
    class Asset;
namespace AssetMananger
{
	template<typename AssetClass>
    std::shared_ptr<Asset> LoadAsset(std::string_view inPath);
}
}