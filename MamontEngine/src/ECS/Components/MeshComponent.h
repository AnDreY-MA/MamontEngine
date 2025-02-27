#pragma once

namespace MamontEngine
{
    struct Mesh;
	struct MeshComponent
	{
        MeshComponent() = default;
        MeshComponent(const MeshComponent &) = default;

        explicit MeshComponent(const std::shared_ptr<Mesh> &inMesh) : Mesh(inMesh)
        {
            Dirty = true;
        }

        std::shared_ptr<Mesh> Mesh;

        bool Dirty{false};
	};
}