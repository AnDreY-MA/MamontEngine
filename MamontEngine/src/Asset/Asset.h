#pragma once

#include <filesystem>
#include <cereal/cereal.hpp>
#include <cereal/archives/portable_binary.hpp>
namespace MamontEngine
{
    enum class EAssetType : uint8_t
    {
        None, Model, Audio, Material, Texture
    };

	class Asset
	{
    public:
        Asset() : m_Type(EAssetType::None)
        {
        }

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

        EAssetType GetType() const { return m_Type; }

    protected:
        Asset(std::string_view inPath) : m_FilePath(inPath.data())
        {

        }

	protected:
        std::filesystem::path m_FilePath;
        std::string           m_Name{std::string("")};

        EAssetType m_Type;

	private:
        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {
            ar(cereal::make_nvp("filePath", m_FilePath));
        }
	};
}