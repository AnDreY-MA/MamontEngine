#include "Graphics/Resources/Textures/TextureManager.h"
#include <Graphics/Resources/Texture.h>
#include "Core/Engine.h"

namespace MamontEngine
{
    void TextureManager::Init()
    {
        m_Textures.reserve(100);

        auto whiteTexture = std::shared_ptr<Texture>(CreateWhiteTexture());
        m_WhiteTextureIndex = AddTexture(whiteTexture);
    }
    
    void TextureManager::Shutdown()
    {
        m_Textures.clear();
    }

    uint32_t TextureManager::AddTexture(std::shared_ptr<Texture> &inTexture)
    {
        const uint32_t textureId = static_cast<uint32_t>(m_Textures.size());

        auto& contextDevice = MEngine::Get().GetContextDevice();
        contextDevice.UpdateBindlessDescriptorSet(inTexture->GetDescriptorInfo(), textureId);
        inTexture->SetBindId(textureId);

        m_Textures.push_back(inTexture);

        return textureId;
    }

    void TextureManager::RemoveTexture()
    {
        m_Textures.erase(m_Textures.begin(), m_Textures.end());
    }
    
} // namespace MamontEngine
