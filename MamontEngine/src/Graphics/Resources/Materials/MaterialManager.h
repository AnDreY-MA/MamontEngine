#pragma once

#include "Material.h"

namespace MamontEngine
{
	class MaterialManager
	{
    public:
        explicit MaterialManager();
        ~MaterialManager();

		static MaterialManager *Get() { return s_Instance; }

		std::shared_ptr<Material>
        CreateMaterial(const EMaterialPass pass, const Material::MaterialResources &resources, const Material::MaterialConstants &data);

	private:
        std::vector<std::shared_ptr<Material>> m_Materials;

		static MaterialManager *s_Instance;
	};
}