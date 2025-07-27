#pragma once

#include <glm/gtx/transform.hpp>
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace MamontEngine
{
	class MUI
	{
    public:
        static char IDBuffer[16];

        static void PushID();
        static void PopID();

        static void PushFrameStyle(const bool on = true);
        static void PopFrameStyle();

        static constexpr ImGuiTableFlags DefaultPropertiesFlags = ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchSame;
        static bool BeginProperties(ImGuiTableFlags inFlags = DefaultPropertiesFlags, const bool inFixedWidth = false, const float inWidth = 0.4f);
        static void EndProperties();

        static void BeginPropertyGrid(std::string_view inLabel, std::string_view inTooltip, const bool inAlignTextRight = false);
        static void EndPropertyGrid();
        
        static bool DrawVec3Control(std::string_view inLabel, glm::vec3& inValues, float inResetValue = 0.0f);

		static bool InputText(const char            *inLabel,
                              std::string           *str,
                              size_t                 bufSize = 256,
                              ImGuiInputTextFlags    flags    = 0,
                              ImGuiInputTextCallback callback = NULL,
                              void                  *userData = NULL);
	};
}