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

		~Editor();

		static Editor *Get();

		virtual void Init() override;

		virtual void Deactivate() override;

		virtual void ImGuiRender() override;

		MamontEngine::Entity GetSelectedEntity()
        {
            return GetPanel<SceneHierarchyPanel>()->GetSelectedEntity();
        }

		std::shared_ptr<MamontEngine::Scene>& GetSceneContext()
		{
            return GetPanel<SceneHierarchyPanel>()->GetSceneContext();
		}

	protected:
        virtual void Begin() override;
        virtual void End() override;

    private:
		
		template<typename T>
		void AddPanel()
		{
            m_Panels.emplace(typeid(T).hash_code(), std::make_unique<T>());
		}

		template <typename T>
		T* GetPanel()
		{
            const size_t hashCode = typeid(T).hash_code();
            return (T *)(m_Panels[hashCode].get());
		}

		void DrawMainPanel();

		void PickObject(const ImVec2& inMousePostion);

		void AddIconFont();

	private:
        std::unordered_map<size_t, std::unique_ptr<EditorPanel>> m_Panels;

		glm::vec2 m_ViewportBounds[2];

		static Editor* s_Instance;
	};
}