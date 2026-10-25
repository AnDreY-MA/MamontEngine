#include "Audio/AudioSystem.h"

#include "Core/Log.h"

#include "soloud/include/soloud_wav.h"
#include <soloud/include/soloud.h>
#include "Audio/Sound.h"

namespace MamontEngine
{
    constexpr int  AudioSampleRate = 48000;
    constexpr int  AudioFrameSize  = 512;

    AudioSystem::AudioSystem()
    {
        m_Soloud              = std::make_unique<SoLoud::Soloud>();
        const auto initResult = m_Soloud->init(SoLoud::Soloud::CLIP_ROUNDOFF, SoLoud::Soloud::MINIAUDIO,
                                        AudioSampleRate,
                                        AudioFrameSize);


        Log::Info("Soloud init: {}", !initResult);

        m_Soloud->setGlobalVolume(1.0f);
        m_Soloud->setMaxActiveVoiceCount(128);
    }

    AudioSystem::~AudioSystem()
    {
        for (auto& wavs : m_Sounds)
        {
            wavs.second->stop();
            wavs.second.reset();
        }
        
        m_Soloud->deinit();
    }

    void AudioSystem::Update()
    {
        m_Soloud->update3dAudio();
    }

    std::shared_ptr<Sound> AudioSystem::PlaySound(std::string_view inFile, bool isLooping)
    {
        const std::string filePath{inFile};
        Log::Info("Play Sound: {}", filePath);

        if (!LoadSound(inFile))
        {
            Log::Warn("Error loading wav: {}", inFile.data());
            return nullptr;
        }

        auto &wav = m_Sounds[filePath];
        wav->setLooping(isLooping);
        const auto handle = m_Soloud->play(*wav, 1.f);
        Log::Info("Sound handle: {}", handle);

        std::shared_ptr<Sound> sound = std::make_shared<Sound>(filePath, handle);

        return sound;
    }

    void AudioSystem::StopSound(std::string_view inFile)
    {
        if (auto wav = m_Sounds.find(inFile.data()); wav != m_Sounds.end())
        {
            wav->second->stop();
            m_Sounds.erase(wav);
        }
    }

    void AudioSystem::StopAll()
    {
        m_Soloud->stopAll();
        m_Sounds.clear();
    }

    bool AudioSystem::LoadSound(std::string_view inFilePath)
    {
        const std::string filePath{inFilePath};

        if (auto it = m_Sounds.find(filePath); it != m_Sounds.end() && it->second)
        {
            return true;
        }

        auto       wav    = std::make_shared<SoLoud::Wav>();
        const auto result = wav->load(filePath.c_str()); 
        if (result != SoLoud::SO_NO_ERROR)
        {
            Log::Error("Error loading wav '{}': code {}", filePath, static_cast<int>(result));
            return false;
        }

        m_Sounds.emplace(filePath, std::move(wav));
        return true;
    }
} // namespace MamontEngine
