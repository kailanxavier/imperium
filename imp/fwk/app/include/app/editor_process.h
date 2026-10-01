#pragma once

#include <core/types/int_types.h>
#include <filesystem>

namespace imp::app
{
	[[nodiscard]] std::filesystem::path resolveEditorPath(const std::filesystem::path& hint);
	[[nodiscard]] u32 currentProcessId();

	class EditorProcess
	{
	public:
		EditorProcess() = default;
		~EditorProcess();

		EditorProcess(const EditorProcess&) = delete;
		EditorProcess& operator=(const EditorProcess&) = delete;

		bool launch(const std::filesystem::path& executable, u16 port, bool embedded);
		[[nodiscard]] bool launched() const { return m_process != nullptr; }

		[[nodiscard]] bool running() const;

	private:
		void* m_process = nullptr;
	};
}
