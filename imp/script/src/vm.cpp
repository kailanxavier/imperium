#include "script/vm.h"
#include <sol/sol.hpp>

namespace imp::script
{
	struct ScriptVM::Impl
	{
		sol::state lua;
	};

	ScriptVM::ScriptVM() : m_impl(std::make_unique<Impl>())
	{
		m_impl->lua.open_libraries(
			sol::lib::base,
			sol::lib::string,
			sol::lib::math,
			sol::lib::table
		);
	}

	ScriptVM::~ScriptVM() = default;

	ScriptVM::ScriptVM(ScriptVM&&) noexcept = default;
	ScriptVM& ScriptVM::operator=(ScriptVM&&) noexcept = default;

	bool ScriptVM::isValid() const noexcept
	{
		return m_impl != nullptr;
	}

	bool ScriptVM::execute(const std::string& source, std::string* outError)
	{
		const sol::protected_function_result result = m_impl->lua.script(source, sol::script_pass_on_error);
		if (result.valid())
			return true;

		if (outError)
		{
			const sol::error error = result;
			*outError = error.what();
		}

		return false;
	}

	void ScriptVM::bindIntFunction(const std::string& name, std::function<int(int)> fn)
	{
		m_impl->lua.set_function(name, std::move(fn));
	}
}
