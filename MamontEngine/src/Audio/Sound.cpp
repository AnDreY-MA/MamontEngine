#include "Audio/Sound.h"
#include "Utils/Reflection.h"
#include "Core/Engine.h"
#include "Core/Log.h"

namespace MamontEngine
{
    /*IMPLEMENT_REFLECT_OBJECT(Sound)
    {
        meta.base<Asset>();
        meta.data<&Sound::m_FilePath>("File");
    }
    FINISH_REFLECT()*/

    Sound::Sound()
    {
        m_Type = EAssetType::Audio;
    }

    Sound::Sound(std::string_view inFile, SoundHandle inHandle) 
        : Asset(inFile), m_Handle(inHandle)
    {
        m_Type = EAssetType::Audio;
    }

    Sound::~Sound()
    {
        Log::Info("Sound desctruction");
    }

    void Sound::Load(std::string_view filePath)
    {
        m_FilePath = filePath.data();
        MEngine::Get().GetAudioSystem().LoadSound(filePath);
    }


}