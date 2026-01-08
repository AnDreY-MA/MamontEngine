#pragma once

#include <filesystem>

namespace MamontEngine
{
	class Asset
	{
    public:
        virtual ~Asset() = default;

		virtual void Load(std::string_view filePath){}

		const std::string &GetPathFile() const
        {
            return m_FilePath.string();
        }

		const std::string& GetName() const
		{
            return m_Name;
		}

	protected:
        std::filesystem::path m_FilePath;
        std::string           m_Name{std::string("")};
	};
}