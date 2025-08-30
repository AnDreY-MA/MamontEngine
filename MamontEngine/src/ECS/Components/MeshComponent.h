#pragma once

namespace MamontEngine
{
    struct MeshModel;

	struct MeshComponent
	{
        MeshComponent() = default;
        MeshComponent(const MeshComponent &) = default;

        explicit MeshComponent(const std::shared_ptr<MeshModel> &inMesh) : Mesh(inMesh)
        {
            Dirty = true;
        }

        std::shared_ptr<MeshModel> Mesh;

        bool Dirty{false};
	};
}