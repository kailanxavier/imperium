#include <gfx/shader_compiler.h>

#include <system_error>
#include <cstdio>
#include <array>

#if defined(_WIN32)
#define IMP_POPEN _popen
#define IMP_PCLOSE _pclose
#else
#define IMP_POPEN popen
#define IMP_PCLOSE pclose
#endif

namespace imp::gfx
{
    namespace
    {
        std::string quote(const std::filesystem::path& path)
        {
            return "\"" + path.generic_string() + "\"";
        }
    }

    ShaderCompiler::ShaderCompiler(std::string compilerPath, bool usesGlslangValidator)
        : m_compilerPath(std::move(compilerPath))
        , m_usesGlslangValidator(usesGlslangValidator)
    {
    }

    bool ShaderCompiler::compile(const std::filesystem::path &source, const std::filesystem::path &outSpirv, std::string &outErrorLog) const
    {
        outErrorLog.clear();
        if (!isValid())
        {
            outErrorLog = "No GLSL->SPIR-V compiler is configured";
            return false;
        }

        std::error_code ec;
        std::filesystem::create_directories(outSpirv.parent_path(), ec);

        std::string command = quote(m_compilerPath) + " ";
        command += m_usesGlslangValidator
            ? ("-V " + quote(source) + " -o " + quote(outSpirv))
            : (quote(source) + " -o " + quote(outSpirv));
        command += " 2>&1";

        FILE* pipe = IMP_POPEN(command.c_str(), "r");
        if (!pipe)
        {
            outErrorLog = "Failed to launch shader compiler process";
            return false;
        }

        std::array<char, 512> buffer{};
        while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe))
            outErrorLog += buffer.data();

        const int exitCode = IMP_PCLOSE(pipe);
        return exitCode == 0;
    }
}
