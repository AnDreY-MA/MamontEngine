#pragma once

#include "EditorPanel.h"

namespace MamontEditor
{
	class StatisticsPanel final : public EditorPanel
	{
    public:
        explicit StatisticsPanel(const std::string& inName);

        ~StatisticsPanel() = default;

		virtual void Deactivate() override;

        virtual void Init() override;

        virtual void GuiRender() override;
        
	};
}