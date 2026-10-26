#pragma once

#include "Graphics/Resources/Models/Model.h"
#include "ECS/Components/Component.h"
#include <cereal/cereal.hpp>
#include "Utils/MetaReflection.h"

namespace MamontEngine
{
	struct MeshComponent : public Component
	{
        explicit MeshComponent()
        {
            Mesh = std::make_shared<Model>();
        }
        MeshComponent(const MeshComponent &) = default;

        explicit MeshComponent(const std::shared_ptr<Model> &inMesh) : Mesh(inMesh)
        {
            Dirty = true;
        }

        std::shared_ptr<Model> Mesh;

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

            component.Mesh = std::make_shared<Model>(1, path);
        }

        REFLECT();
	};
}