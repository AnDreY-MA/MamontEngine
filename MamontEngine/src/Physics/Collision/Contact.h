#pragma once

namespace MamontEngine
{
	namespace HeroPhysics
	{
        class Rigidbody;

		struct Contact
		{
            Rigidbody *body1{nullptr};
            Rigidbody *body2{nullptr};

			glm::vec3 Point;
			glm::vec3 Normal;
            float     PenetrationDepth{0};
		};

		//Contact FindContact(const Rigidbody *inBody1, const Rigidbody *inBody2);
	}
}