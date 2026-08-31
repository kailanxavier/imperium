#include <core/config/cvar.h>
#include <core/log/log.h>
#include <fstream>
#include <sstream>

namespace imp
{
	CVarRegistry& CVarRegistry::get()
	{
		static CVarRegistry instance;
		return instance;
	}

	float& CVarRegistry::registerFloat(std::string_view name, float defaultValue)
	{
		return m_floats.try_emplace(std::string(name), defaultValue).first->second;
	}

	i32& CVarRegistry::registerInt(std::string_view name, i32 defaultValue)
	{
		return m_ints.try_emplace(std::string(name), defaultValue).first->second;
	}

	bool& CVarRegistry::registerBool(std::string_view name, bool defaultValue)
	{
		return m_bools.try_emplace(std::string(name), defaultValue).first->second;
	}

	std::vector<CVarSnapshot> CVarRegistry::list() const
	{
		std::vector<CVarSnapshot> entries;
		entries.reserve(m_floats.size() + m_ints.size() + m_bools.size());

		for (const auto& [name, value] : m_floats)
		{
			CVarSnapshot s;
			s.name = name;
			s.kind = CVarKind::Float;
			s.floatValue = value;
			entries.push_back(std::move(s));
		}

		for (const auto& [name, value] : m_ints)
		{
			CVarSnapshot s;
			s.name = name;
			s.kind = CVarKind::Int;
			s.intValue = value;
			entries.push_back(std::move(s));
		}

		for (const auto& [name, value] : m_bools)
		{
			CVarSnapshot s;
			s.name = name;
			s.kind = CVarKind::Bool;
			s.boolValue = value;
			entries.push_back(std::move(s));
		}

		return entries;
	}

	std::optional<CVarSnapshot> CVarRegistry::get(std::string_view name) const
	{
		const std::string key(name);

		if (auto it = m_floats.find(key); it != m_floats.end())
		{
			CVarSnapshot s;
			s.name = key;
			s.kind = CVarKind::Float;
			s.floatValue = it->second;
			return s;
		}

		if (auto it = m_ints.find(key); it != m_ints.end())
		{
			CVarSnapshot s;
			s.name = key;
			s.kind = CVarKind::Int;
			s.intValue = it->second;
			return s;
		}

		if (auto it = m_bools.find(key); it != m_bools.end())
		{
			CVarSnapshot s;
			s.name = key;
			s.kind = CVarKind::Bool;
			s.boolValue = it->second;
			return s;
		}

		return std::nullopt;
	}

	bool CVarRegistry::setFloat(std::string_view name, float value)
	{
		auto it = m_floats.find(std::string(name));
		if (it == m_floats.end())
			return false;

		it->second = value;
		return true;
	}

	bool CVarRegistry::setInt(std::string_view name, i32 value)
	{
		auto it = m_ints.find(std::string(name));
		if (it == m_ints.end())
			return false;

		it->second = value;
		return true;
	}

	bool CVarRegistry::setBool(std::string_view name, bool value)
	{
		auto it = m_bools.find(std::string(name));
		if (it == m_bools.end())
			return false;

		it->second = value;
		return true;
	}

	bool CVarRegistry::loadFromFile(std::string_view path)
	{
		std::ifstream file{ std::string(path) };
		if (!file.is_open())
			return false;

		std::string line;
		while (std::getline(file, line))
		{
			if (line.empty() || line[0] == '#')
				continue;

			const auto eq = line.find('=');
			if (eq == std::string::npos)
				continue;

			const std::string key = line.substr(0, eq);
			const std::string valueStr = line.substr(eq + 1);

			if (auto it = m_floats.find(key); it != m_floats.end())
				it->second = std::stof(valueStr);
			else if (auto it2 = m_ints.find(key); it2 != m_ints.end())
				it2->second = std::stoi(valueStr);
			else if (auto it3 = m_bools.find(key); it3 != m_bools.end())
				it3->second = ( valueStr == "true" || valueStr == "1" );
			else
				LOG_WARN("CVar", "Unknown cvar '{}' in {}, ignoring", key, path);
		}
		return true;
	}
}
