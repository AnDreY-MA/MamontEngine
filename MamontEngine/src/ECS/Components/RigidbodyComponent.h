#pragma once 

#include "Utils/MetaReflection.h"
#include <entt/entt.hpp>
#include <cereal/cereal.hpp>

namespace MamontEngine
{
    namespace HeroPhysics
    {
        class Rigidbody;
	}
	
	struct RigidbodyComponent
	{
        REFLECT()
    public:
        RigidbodyComponent();

		~RigidbodyComponent();

        HeroPhysics::Rigidbody *Rigidbody;

    private:
        friend class cereal::access;
        
	};

    template <typename Archive>
    void serialize(Archive &ar, RigidbodyComponent &component)
    {

    }
}