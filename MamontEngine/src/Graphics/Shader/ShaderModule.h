#pragma once
#include "ShaderCompiler.h"

namespace MamontEngine
{
	class ShaderModule
	{
    public:
        ShaderModule(VkDevice device, std::string_view path, EShaderStage stage);
        ~ShaderModule();

        inline VkShaderModule GetModule() const { return m_Module; }

	private:
        VkShaderModule m_Module{VK_NULL_HANDLE};
        VkDevice       m_Device;
	};
}