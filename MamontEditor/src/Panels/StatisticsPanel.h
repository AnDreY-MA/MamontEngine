#pragma once

#include "EditorPanel.h"

namespace MamontEditor
{
	class StatisticsPanel final : public EditorPanel
	{
    public:
        explicit StatisticsPanel(const std::string &inName = "Stats");

        ~StatisticsPanel() = default;

        virtual void GuiRender() override;

        virtual bool IsHovered() const override;
        
	};
}