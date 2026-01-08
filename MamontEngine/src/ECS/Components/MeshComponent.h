#pragma once

#include "Graphics/Resources/Models/Model.h"
#include "ECS/Components/Component.h"

namespace MamontEngine
{
	struct MeshComponent : public Component
	{
        MeshComponent() = default;
        MeshComponent(const MeshComponent &) = default;

        MeshComponent(const std::shared_ptr<MeshModel> &inMesh) : Mesh(inMesh)
        {
            Dirty = true;
        }

        std::shared_ptr<MeshModel> Mesh;

        bool Dirty{false};
	};
}