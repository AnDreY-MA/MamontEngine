#pragma once

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

	};
}