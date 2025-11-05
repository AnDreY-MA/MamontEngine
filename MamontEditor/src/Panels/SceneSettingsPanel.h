#include <memory>
#include "EditorPanel.h"

#include <ECS/Entity.h>
#include <ECS/Scene.h>

namespace MamontEditor
{
	class SceneSettingsPanel final : public EditorPanel
	{
    public:
        explicit SceneSettingsPanel(const std::string &inName = "Scene Settings");

        ~SceneSettingsPanel() = default;

        virtual void Deactivate() override;

        virtual void Init() override;

        virtual void GuiRender() override;

	};
}
