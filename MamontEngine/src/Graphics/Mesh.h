#pragma once 

#include "Graphics/Vulkan/GPUBuffer.h"

namespace MamontEngine
{
    struct Vertex
    {
        glm::vec3 Position = {0.f, 0.f, 0.f};
        float     UV_X{0.f};
        glm::vec3 Normal = {0.f, 0.f, 0.f};
        float     UV_Y{0.f};
        glm::vec4 Color;
    };

	struct MeshTest
	{
        std::string             Name;
        std::vector<GeoSurface> Surfaces;
        GPUMeshBuffers          MeshBuffers;
	};

	class Mesh
    {
    public:

    private:
        std::vector<Vertex> Vertices;
    };
}