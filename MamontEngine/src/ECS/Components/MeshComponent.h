#pragma once

namespace MamontEngine
{
    struct MeshNode;

	struct MeshComponent
	{
        MeshComponent() = default;
        explicit MeshComponent(const std::shared_ptr<MeshNode> &inMesh) : Mesh(inMesh)
        {
        }

        std::shared_ptr<MeshNode> Mesh{nullptr};
	};
}