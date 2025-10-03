#pragma once

#include "Graphics/Model.h"

namespace MamontEngine
{
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