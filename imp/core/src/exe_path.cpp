#include "core/platform/exe_path.h"

#ifdef _WIN32
#include <windows.h>
#elifdef __APPLE__
#include <mach-o/dyld.h>
#include <vector>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace imp::platform
{
    std::filesystem::path executableDir()
    {
        static const std::filesystem::path dir = []
        {
#ifdef _WIN32
            wchar_t buf[MAX_PATH];
            DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
            return std::filesystem::path(buf, buf + len).parent_path();
#elifdef __APPLE__
            uint32_t size = 0;
            _NSGetExecutablePath(nullptr, &size);
            std::vector<char> buf(size);
            _NSGetExecutablePath(buf.data(), &size);
            return std::filesystem::path(buf.data()).parent_path();
#else
            char buf[PATH_MAX];
            ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
            if (len <= 0) return std::filesystem::current_path(); // fallback
            buf[len] = '\0';
            return std::filesystem::path(buf).parent_path();
#endif
        }();
        return dir;
    }
}
