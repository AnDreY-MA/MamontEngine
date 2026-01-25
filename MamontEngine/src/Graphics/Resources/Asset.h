#pragma once

#include <filesystem>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
namespace MamontEngine
{

	class Asset
	{
    public:
        Asset()          = default;
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

	private:
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            //ar(cereal::make_nvp("filePath", m_FilePath));
        }
	};
}