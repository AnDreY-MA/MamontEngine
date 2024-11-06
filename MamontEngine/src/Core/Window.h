#pragma once

namespace MamontEngine
{
	struct MWindow
	{
        void Init(std::string_view inName, const VkExtent2D &inExtent = {1700, 900});

		void Update();

		void Shutdown();

		void *WindowHandle{nullptr};

		bool IsRequestResize{false};
	};
}