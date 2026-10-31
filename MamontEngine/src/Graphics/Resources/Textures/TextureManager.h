#pragma once

#include "Core/UID.h"

namespace MamontEngine
{
    class Texture;

	class TextureManager
	{
    public:
		static TextureManager& GetInstance()
		{
            static TextureManager instance;
            return instance;
		}

		void Init();
        void Shutdown();

		uint32_t AddTexture(std::shared_ptr<Texture> &inTexture);
        void     RemoveTexture();

		std::weak_ptr<Texture> GetTexture(uint32_t id)
        {
            return m_Textures[id];
        }

		//inline uint32_t GetErrorTexture() const { return m_ErrorTextureIndex; }
		inline uint32_t GetWhiteTexture() const { return m_WhiteTextureIndex; }

	private:
        std::vector<std::shared_ptr<Texture>> m_Textures;

		uint32_t m_ErrorTextureIndex{0};

		uint32_t m_WhiteTextureIndex{0};
	};
}