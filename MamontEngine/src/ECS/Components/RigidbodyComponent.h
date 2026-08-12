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

        RigidbodyComponent(RigidbodyComponent &&other) noexcept 
            : Rigidbody(std::exchange(other.Rigidbody, nullptr))
        {
        }
        RigidbodyComponent &operator=(RigidbodyComponent &&other) noexcept
        {
            if (this != &other)
            {
                Rigidbody = std::exchange(other.Rigidbody, nullptr);
            }
            return *this;
        }

        RigidbodyComponent(const RigidbodyComponent &)            = delete;
        RigidbodyComponent &operator=(const RigidbodyComponent &) = delete;

        HeroPhysics::Rigidbody *Rigidbody;

    private:
        friend class cereal::access;
        
	};

    template <typename Archive>
    void serialize(Archive &ar, RigidbodyComponent &component)
    {

    }
}