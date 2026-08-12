#include "Physics/Body/Rigidbody.h"
#include "Physics/Collision/CollisionShape.h"
#include "Utils/Reflection.h"

namespace MamontEngine
{
namespace HeroPhysics
{
    REFLECT_ENUM(MamontEngine::HeroPhysics::EMotionType)

    IMPLEMENT_REFLECT_ENUM(MamontEngine::HeroPhysics::EMotionType)
    {
        meta.data<MamontEngine::HeroPhysics::EMotionType::Static>("Static"_hs, "Static");
        meta.data<MamontEngine::HeroPhysics::EMotionType::Dynamic>("Dynamic"_hs, "Dynamic");
    }
    FINISH_REFLECT()

    IMPLEMENT_REFLECT_OBJECT(Rigidbody)
    {
        meta.data<&Rigidbody::m_Mass>("Mass");
        meta.data<&Rigidbody::m_GravityScale>("Gravity Scale");
        meta.data<&Rigidbody::m_Sleep>("Sleep");
        meta.data<&Rigidbody::m_MotionType>("Motion Type");
    }
    FINISH_REFLECT()

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