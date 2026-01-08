#include <memory>
#include "EditorPanel.h"

#include <ECS/Scene.h>

namespace MamontEditor
{
    class SceneRenderer;

	class SceneSettingsPanel final : public EditorPanel
	{
    public:
        explicit SceneSettingsPanel(const std::string &inName = "Scene Settings");

        ~SceneSettingsPanel() = default;

        virtual void GuiRender() override;

    private:


	};
}