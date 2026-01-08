#pragma once 

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "Math/Transform.h"
#include "ECS/Components/Component.h"
#include "Utils/MetaReflection.h"
#include "Graphics/Resources/Models/Model.h"

namespace MamontEngine
{
	struct TransformComponent : public Component
	{
        TransformComponent();

        glm::mat4 Matrix() const 
        {
            return Transform.Matrix();
        }
        
        Transform Transform;

        bool IsDirty{true};

     private:
        //REFLECT()
	};
}