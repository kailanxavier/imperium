#pragma once

#include <functional>
#include <memory>
#include <string>

namespace imp::script
{
	class ScriptVM
	{
	public:
		ScriptVM();
		~ScriptVM();

		ScriptVM(const ScriptVM&) = delete;
		ScriptVM& operator=(const ScriptVM&) = delete;

		ScriptVM(ScriptVM&&) noexcept;
		ScriptVM& operator=(ScriptVM&&) noexcept;

		[[nodiscard]] bool isValid() const noexcept;
		bool execute(const std::string& source, std::string* outError = nullptr);

		void bindIntFunction(const std::string& name, std::function<int(int)> fn);

	private:
		struct Impl;
		std::unique_ptr<Impl> m_impl;
	};
}
