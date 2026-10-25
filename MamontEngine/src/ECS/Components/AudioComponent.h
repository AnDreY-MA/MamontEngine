#pragma once

#include "ECS/Components/Component.h"

namespace MamontEngine
{
    class Sound;
    class AudioSystem;

	struct AudioComponent : public Component
	{
    private:
        REFLECT();

    public:
        AudioComponent();
        ~AudioComponent();

        void Play();
        void Pause();
        void Stop();

	private:
        AudioSystem           &m_AudioSystem;

        std::shared_ptr<Sound> m_Sound;

        bool IsLoop{false};
	};
}