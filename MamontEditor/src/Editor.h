#pragma once 
#include <memory>
#include "Core/ImGuiLayer.h"

#include <Panels/SceneHierarchyPanel.h>
#include <Panels/InspectorPanel.h>
#include <Panels/StatisticsPanel.h>

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
        std::unique_ptr<StatisticsPanel>     m_StatsPanel;

		static Editor* s_Instance;
	};
}