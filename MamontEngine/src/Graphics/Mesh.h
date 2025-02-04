#pragma once 

#include "Graphics/Vulkan/GPUBuffer.h"

namespace MamontEngine
{
	struct Mesh
	{
        std::string             Name;
        std::vector<GeoSurface> Surfaces;
        GPUMeshBuffers          MeshBuffers;
	};
}