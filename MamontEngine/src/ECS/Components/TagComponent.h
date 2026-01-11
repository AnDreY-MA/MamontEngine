#pragma once

#include <cereal/cereal.hpp>

namespace MamontEngine
{
	struct TagComponent
	{
        TagComponent() = default;
        TagComponent(const TagComponent &) = default;
        TagComponent(const std::string_view &inTag) : Tag(inTag)
        {
        
        }
        std::string Tag;

    private:
        friend class cereal::access;

        template <typename Archive>
        void serialize(Archive &ar)
        {
            ar(cereal::make_nvp("Name", Tag));
        }
	};

    
}