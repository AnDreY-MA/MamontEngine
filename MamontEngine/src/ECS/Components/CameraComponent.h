#pragma once

#include "Component.h"
#include "Core/Camera.h"
#include <cereal/cereal.hpp>

namespace MamontEngine
{
    struct CameraComponent : public Component
	{
    private:
        REFLECT()

    public:
        explicit CameraComponent();
        ~CameraComponent();

		inline bool IsViewTarget() const { return m_IsViewTarget; }

        inline glm::mat4 GetView() const { return m_Camera->GetViewMatrix(); }
        inline glm::mat4 GetProjection() const { return m_Camera->GetProjection(); }

        inline float GetNear() const { return m_Camera->GetNearClip(); }
        inline float GetFar() const { return m_Camera->GetFarClip(); }

        inline float GetFOV() const { return m_Camera->GetFOV(); } 

        void SetFOV(float inFOV)
        {
            m_Camera->SetFOV(inFOV);
        }

        void UpdateProj(const VkExtent2D &inWindowExtent)
        {
            m_Camera->UpdateProjection(inWindowExtent);
        }

        inline Camera *GetCamera()
        {
            return m_Camera.get();
        }

	private:
        std::shared_ptr<Camera> m_Camera;

		bool m_IsViewTarget{false};

        friend class cereal::access;

        template <class Archive>
        void serialize(Archive &ar)
        {

        }
	};
}