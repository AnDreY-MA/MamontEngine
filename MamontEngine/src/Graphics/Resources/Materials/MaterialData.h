#pragma once

namespace MamontEngine
{
    struct Texture;

	struct MaterialData
	{
        std::shared_ptr<Texture> ColorTexture;
        std::shared_ptr<Texture> MetalRoughTexture;
        std::shared_ptr<Texture> NormalTexture;
        std::shared_ptr<Texture> EmissiveTexture;
        std::shared_ptr<Texture> OcclusionTexture;

        glm::vec4 ColorFactors = glm::vec4(1.0f);

        float MetalicFactor{0.f};
        float RoughFactor{0.f};
	};
}