#pragma once 
#include <Panels/SceneHierarchyPanel.h>
#include <memory>
#include "Core/ImGuiLayer.h"
#include <Panels/InspectorPanel.h>

namespace MamontEditor
{
	class Editor : public MamontEngine::ImGuiLayer
	{
    public:
        explicit Editor();

		~Editor() = default;

		static Editor *Get();

		virtual void Init() override;

		virtual void Deactivate() override;

		virtual void ImGuiRender() override;

		MamontEngine::Entity GetSelectedEntity()
        {
            return m_SceneHierarchy->GetSelectedEntity();
        }

		MamontEngine::Scene* GetSceneContext()
		{
            return m_SceneHierarchy->GetSceneContext();
		}

	private:
        std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchy;
        std::unique_ptr<InspectorPanel>      m_Inspector;

		static Editor* s_Instance;
	};
}