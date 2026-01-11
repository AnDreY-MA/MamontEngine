#pragma once

#include "Core/UID.h"
#include <cereal/cereal.hpp>


namespace MamontEngine
{
	struct IDComponent
	{
        IDComponent() = default;
        IDComponent(const IDComponent &) = default;

		UID ID;

    private:
        friend class cereal::access;

       template <typename Archive>
        void save(Archive &ar) const
        {
           uint64_t id = static_cast<uint64_t>(ID);
            ar(id);
        }
        template <typename Archive>
        void load(Archive &ar)
        {
            uint64_t id = 0;
            ar(id);

            ID = UID(id);
        }


	};

	
}