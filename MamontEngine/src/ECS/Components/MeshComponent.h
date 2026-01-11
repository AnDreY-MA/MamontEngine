#pragma once

#include "Graphics/Resources/Models/Model.h"
#include "ECS/Components/Component.h"
#include <cereal/cereal.hpp>

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

    private:
        friend class cereal::access;

        template <typename Archive>
        friend void save(Archive &archive, const MeshComponent &component)
        {
            archive(cereal::make_nvp("FilePath", component.Mesh->GetPathFile()));
        }
        template <typename Archive>
        friend void load(Archive &archive, MeshComponent &component)
        {
            std::string path;
            archive(cereal::make_nvp("FilePath", path));

            component.Mesh = std::make_shared<MeshModel>(1, path);
        }
	};
}