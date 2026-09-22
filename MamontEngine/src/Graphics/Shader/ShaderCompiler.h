#pragma once

namespace MamontEngine
{
	enum class EShaderStage : uint8_t
	{
		Vertex, Fragment, Compute, Geometry
	};

	enum class EShaderOptimizationLevel : uint8_t
	{
		Node, Size, Performance
	};

	struct ShaderCompilationOptions
	{
        EShaderStage Stage;
		EShaderOptimizationLevel OptimizationLevel;

		std::string Prepend;
        std::string Append;
	};

	struct ShaderCompilationResult
	{
        std::vector<uint32_t> SPIRV;
        std::vector<std::string> Files;
	};

	std::optional<ShaderCompilationResult> CompileShaderFromFile(std::string_view inPath, ShaderCompilationOptions inOptions);
}