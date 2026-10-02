#include <app/editor_process.h>

#include <core/log/log.h>
#include <cstdio>
#include <string>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace imp::app
{
	namespace
	{
		std::string readEnv(const char* name)
		{
#ifdef _WIN32
			char* value = nullptr;
			size_t length = 0;
			if (_dupenv_s(&value, &length, name) != 0 || !value)
				return {};

			std::string result(value);
			free(value);
			return result;
#else
			const char* value = std::getenv(name);
			return value ? value : "";
#endif
		}

		bool isFile(const std::filesystem::path& path)
		{
			std::error_code ec;
			return !path.empty() && std::filesystem::is_regular_file(path, ec);
		}
	}

	std::filesystem::path resolveEditorPath(const std::filesystem::path& hint)
	{
		if (isFile(hint))
			return hint;

		if (!hint.empty())
			LOG_WARN("Editor Host", "Editor not found at '{}'. Did you forget to build imp_editor?", hint.string());

#ifndef NDEBUG
		const char* envName = "IMP_EDITOR_D";
#else
		const char* envName = "IMP_EDITOR";
#endif

		const std::string fromEnv = readEnv(envName);
		if (fromEnv.empty())
		{
			LOG_WARN("Editor Host", "{} is not set. Did you forget to build imp_editor or run 'setenv.bat'?", envName);
			return {};
		}

		const std::filesystem::path envPath = fromEnv;
		if (!isFile(envPath))
		{
			LOG_WARN("Editor Host", "{} points at '{}', which does not exist.", envName, fromEnv);
			return {};
		}

		return envPath;
	}
#ifdef _WIN32
	u32 currentProcessId()
	{
		return static_cast<u32>( GetCurrentProcessId() );
	}

	EditorProcess::~EditorProcess()
	{
		if (m_process)
			CloseHandle(static_cast<HANDLE>( m_process ));
	}

	bool EditorProcess::launch(const std::filesystem::path& executable, u16 port, bool embedded)
	{
		if (m_process)
			return false;

		std::wstring commandLine = L"\"" + executable.wstring() + L"\" --port " + std::to_wstring(port);
		if (embedded)
			commandLine += L" --embedded";

		const std::wstring workingDir = executable.parent_path().wstring();

		STARTUPINFOW startup{};
		startup.cb = sizeof(startup);
		PROCESS_INFORMATION info{};

		if (!CreateProcessW(executable.c_str(), commandLine.data(),
			nullptr, nullptr, FALSE, 0, nullptr, workingDir.c_str(), &startup, &info))
		{
			LOG_ERROR("Editor Host", "CreateProcessW() failed for '{}' (error {})",
				executable.string(), static_cast<u32>( GetLastError() ));
			return false;
		}

		CloseHandle(info.hThread);
		m_process = info.hProcess;

		LOG_INFO("Editor Host", "Launched editor '{}' (pid {}) on port {}", 
			executable.string(), static_cast<u32>( info.dwProcessId ), port);
		return true;
	}

	bool EditorProcess::running() const
	{
		if (!m_process)
			return false;

		return WaitForSingleObject(static_cast<HANDLE>( m_process ), 0) == WAIT_TIMEOUT;
	}
#else

	u32 currentProcessId() { return 0; }
	EditorProcess::~EditorProcess() = default;
	bool EditorProcess::launch(const std::filesystem::path&, u16, bool)
	{
		LOG_ERROR("Editor Host", "Launching the editor is only supported on Windows");
		return false;
	}

	bool EditorProcess::running() const { return false; }

#endif
}
