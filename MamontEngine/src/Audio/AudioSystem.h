#pragma once

namespace SoLoud
{
    class Soloud;
    class Wav;
}

namespace MamontEngine
{
    class Sound;

	class AudioSystem
	{
    public:
        AudioSystem();
        ~AudioSystem();

        void Update();

		std::shared_ptr<Sound> PlaySound(std::string_view inFile, bool isLooping = false);
        void StopSound(std::string_view inFile);

        void StopAll();
        
        bool LoadSound(std::string_view inFilePath);

	private:
        std::unordered_map<std::string, std::shared_ptr<SoLoud::Wav>> m_Sounds;

        std::unique_ptr<SoLoud::Soloud> m_Soloud;
	};
}