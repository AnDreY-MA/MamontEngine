#pragma once

namespace MamontEngine::Loader
{
	struct GeoSurface
	{
        uint32_t StartIndex;
        uint32_t Count;
	};

	struct MeshAsset
	{
        std::string Name;

		std::vector<GeoSurface> Surfaces;
        GPUMeshBuffers          MeshBuffers;
	};
}