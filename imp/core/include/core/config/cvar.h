#pragma once
#include <core/types/int_types.h>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace imp
{
	enum class CVarKind : u8
	{
		Float = 0,
		Int,
		Bool,
	};

	struct CVarSnapshot
	{
		std::string name;
		CVarKind kind = CVarKind::Float;

		float floatValue = 0.f;
		i32 intValue = 0;
		bool boolValue = false;
	};

	class CVarRegistry
	{
	public:
		static CVarRegistry& get();

		float& registerFloat(std::string_view name, float defaultValue);
		i32& registerInt(std::string_view name, i32 defaultValue);
		bool& registerBool(std::string_view name, bool defaultValue);

		bool loadFromFile(std::string_view path);

		[[nodiscard]] std::vector<CVarSnapshot> list() const;
		[[nodiscard]] std::optional<CVarSnapshot> get(std::string_view name) const;

		bool setFloat(std::string_view name, float value);
		bool setInt(std::string_view name, i32 value);
		bool setBool(std::string_view name, bool value);

	private:
		std::unordered_map<std::string, float> m_floats;
		std::unordered_map<std::string, i32> m_ints;
		std::unordered_map<std::string, bool> m_bools;
	};

	class CVarFloat
	{
	public:
		CVarFloat(std::string_view name, float defaultValue)
			: m_value(CVarRegistry::get().registerFloat(name, defaultValue)) {}
		operator float() const { return m_value; }
		float& ref() { return m_value; }

	private:
		float& m_value;
	};

	class CVarInt
	{
	public:
		CVarInt(std::string_view name, i32 defaultValue)
			: m_value(CVarRegistry::get().registerInt(name, defaultValue)) {}
		operator i32() const { return m_value; }
		i32& ref() { return m_value; }

	private:
		i32& m_value;
	};

	class CVarBool
	{
	public:
		CVarBool(std::string_view name, bool defaultValue) 
		: m_value(CVarRegistry::get().registerBool(name, defaultValue)) {}
		operator bool() const { return m_value; }
		bool& ref() { return m_value; }

	private:
		bool& m_value;
	};
}
