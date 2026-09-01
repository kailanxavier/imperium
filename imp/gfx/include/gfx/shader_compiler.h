#pragma once

#include <string>
#include <filesystem>

namespace imp::gfx
{
    class ShaderCompiler
    {
    public:
        ShaderCompiler(std::string compilerPath, bool usesGlslangValidator);
        [[nodiscard]] bool isValid() const noexcept { return !m_compilerPath.empty(); }

        bool compile(const std::filesystem::path& source,
            const std::filesystem::path& outSpirv,
            std::string& outErrorLog) const;

    private:
        std::string m_compilerPath;
        bool m_usesGlslangValidator = false;
    };
}
