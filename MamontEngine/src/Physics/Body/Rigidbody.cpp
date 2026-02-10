#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/CollisionShape.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
namespace HeroPhysics
{
     /*
    {
        meta.data<&Rigidbody::m_Mass>("Mass");
    }
    FINISH_REFLECT()*/

    Rigidbody::Rigidbody()
    {
        m_Id = UID();
    }

    void Rigidbody::SetCollisionShape(const std::shared_ptr<CollisionShape> &inShape)
    {
        m_Shape = inShape;
    }
}
}