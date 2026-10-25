#pragma once

#include "Asset/Asset.h"
#include "Utils/MetaReflection.h"
#include <filesystem>

namespace SoLoud
{
    class Wav;
}

namespace MamontEngine
{
    typedef unsigned int SoundHandle;

	class Sound : public Asset
	{
        //REFLECT();
    public:
        explicit Sound();
        Sound(std::string_view inFile, SoundHandle inHandle);
        ~Sound();

        virtual void Load(std::string_view filePath) override;

        SoundHandle           m_Handle;
	};
}