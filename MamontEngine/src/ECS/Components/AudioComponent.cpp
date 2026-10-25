#include "ECS/Components/AudioComponent.h"
#include "Audio/Sound.h"

#include "Utils/Reflection.h"
#include "Core/Engine.h"

namespace MamontEngine
{
    IMPLEMENT_REFLECT_COMPONENT(AudioComponent, "AudioComponent")
    {
        meta.base<Component>();
        meta.data<&AudioComponent::m_Sound, entt::as_ref_t>("SoundSource");
        meta.data<&AudioComponent::IsLoop, entt::as_ref_t>("Loop");
    }
    FINISH_REFLECT()

    AudioComponent::AudioComponent() : 
        m_AudioSystem(MEngine::Get().GetAudioSystem())
    {
       m_Sound = std::make_shared<Sound>();
    }

    AudioComponent::~AudioComponent()
    {
        Log::Info("~AudioComponent");
    }
    
    void AudioComponent::Play()
    {
        if (!m_Sound)
        {
            Log::Warn("AudioComponent, Play error: Sound is not valid");
            return;
        }

        const std::string musicPath = DEFAULT_ASSETS_DIRECTORY + "music.ogg";
        auto newSound              = m_AudioSystem.PlaySound(m_Sound->GetPathFile(), IsLoop);
        m_Sound.swap(newSound);
    }
    
    void AudioComponent::Pause()
    {
        if (!m_Sound) return; 

    }
    
    void AudioComponent::Stop()
    {
        if (!m_Sound) return; 

        m_AudioSystem.StopSound(m_Sound->GetPathFile());
    }
}