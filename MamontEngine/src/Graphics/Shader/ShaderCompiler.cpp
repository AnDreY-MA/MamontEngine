#include "ShaderCompiler.h"
#include "Core/Log.h"
#include <shaderc/shaderc.hpp>
#include <algorithm>
#include <ranges>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace
{
    std::string ReadFile(std::string& path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return "";

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    [[nodiscard]] shaderc_optimization_level ToShadercOptimizationLevel(MamontEngine::EShaderOptimizationLevel inLevel)
    {
        using namespace MamontEngine;

        switch (inLevel)
        {
            case MamontEngine::EShaderOptimizationLevel::Node:
                return shaderc_optimization_level_zero;
            case MamontEngine::EShaderOptimizationLevel::Size:
                return shaderc_optimization_level_size;
            case MamontEngine::EShaderOptimizationLevel::Performance:
                return shaderc_optimization_level_performance;
        }
    }

    [[nodiscard]] shaderc_shader_kind ToShadercStage(MamontEngine::EShaderStage inStage)
    {
        using namespace MamontEngine;

        switch (inStage)
        {
            case MamontEngine::EShaderStage::Vertex:
                return shaderc_vertex_shader;
            case MamontEngine::EShaderStage::Fragment:
                return shaderc_fragment_shader;
            case MamontEngine::EShaderStage::Compute:
                return shaderc_compute_shader;
            case MamontEngine::EShaderStage::Geometry:
                return shaderc_geometry_shader;
        }
    }

    struct IncludeData
    {
        std::unique_ptr<shaderc_include_result> IncludeResult;
        std::string                             FileName;
        std::string                             Content;
    };

    class ShaderIncluder final : public shaderc::CompileOptions::IncluderInterface
    {
        std::vector<IncludeData> &m_Includes;

    public:
        ShaderIncluder(std::reference_wrapper<std::vector<IncludeData>> includes) 
            : m_Includes(includes.get())
        {

        }

    private:
        shaderc_include_result* GetInclude(const char* requestSource, shaderc_include_type type, const char* requestingSource, size_t includeDepth) override
        {
            std::string name;
            std::string contents;

            const std::filesystem::path shaderDir = DEFAULT_ASSETS_DIRECTORY + "Shaders";

            std::vector<std::filesystem::path> requestingDirs{shaderDir};

            if (type == shaderc_include_type_relative)
            {
                std::filesystem::path requestDir = requestingSource;
                requestDir.remove_filename();
                requestingDirs.push_back(requestDir);
            }

            for (const auto& requestingDir : requestingDirs | std::views::reverse)
            {
                const auto requested_path = std::filesystem::weakly_canonical(requestingDir / requestSource);
                if (std::filesystem::exists(requested_path))
                {
                    name     = requested_path.string();
                    contents = ReadFile(name);
                    break;
                }
            }

            auto &result = m_Includes.emplace_back(std::make_unique_for_overwrite<shaderc_include_result>(), name, contents);

            *result.IncludeResult = shaderc_include_result {
                .source_name = result.FileName.data(),
                .source_name_length = result.FileName.size(), 
                .content = result.Content.data(),
                .content_length     = result.Content.size(),
            };

            return result.IncludeResult.get();
        }

        void ReleaseInclude(shaderc_include_result *) override {}
    };
}

namespace MamontEngine
{
    std::optional<ShaderCompilationResult> CompileShaderFromFile(std::string_view inPath, ShaderCompilationOptions inOptions)
    {
        const shaderc::Compiler compiler;

        std::string src;
        std::ifstream file(std::string(inPath), std::ios::binary);
        
        std::ostringstream ss;
        ss << file.rdbuf();

        src = ss.str();

        if (not inOptions.Append.empty() || not inOptions.Prepend.empty())
        {
            src = fmt::format("{}{}{}", inOptions.Prepend, src, inOptions.Append);
        }

        shaderc::CompileOptions shadercOptions{};

        std::vector<IncludeData> includes;

        shadercOptions.SetIncluder(std::make_unique<ShaderIncluder>(std::reference_wrapper(includes)));
        shadercOptions.SetOptimizationLevel(ToShadercOptimizationLevel(inOptions.OptimizationLevel));
        shadercOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
        shadercOptions.SetTargetSpirv(shaderc_spirv_version_1_6);

        const auto shadercStage = ToShadercStage(inOptions.Stage);

        const auto compilatioResult = compiler.CompileGlslToSpv(src, shadercStage, inPath.data(), shadercOptions);

        if (compilatioResult.GetCompilationStatus() != shaderc_compilation_status_success)
        {
            Log::Error("Failed to compile {} \n {}", inPath.data(), compilatioResult.GetErrorMessage());
            return std::nullopt;
        }

        std::vector<std::string> includeFiles;
        std::ranges::transform(includes, std::back_inserter(includeFiles), &IncludeData::FileName);

        return ShaderCompilationResult{
            .SPIRV = std::vector(compilatioResult.begin(), compilatioResult.end()), 
            .Files = includeFiles
        };
    }
}