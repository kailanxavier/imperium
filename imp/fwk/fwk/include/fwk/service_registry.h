#pragma once

#include <core/log/log.h>
#include <cstdlib>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace imp::fwk
{
	class ServiceRegistry
	{
	public:
		ServiceRegistry() = default;
		ServiceRegistry(const ServiceRegistry&) = delete;
		ServiceRegistry& operator=(const ServiceRegistry&) = delete;

		template <typename T>
		void provide(T& service)
		{
			m_services[std::type_index(typeid( T ))] = &service;
		}

		template <typename T>
		void remove()
		{
			m_services.erase(std::type_index(typeid( T )));
		}

		template <typename T>
		[[nodiscard]] T* tryGet() const
		{
			const auto it = m_services.find(std::type_index(typeid( T )));
			return it == m_services.end() ? nullptr : static_cast<T*>( it->second );
		}

		template <typename T>
		[[nodiscard]] bool has() const { return tryGet<T>() != nullptr; }

		template <typename T>
		[[nodiscard]] T& require() const
		{
			T* service = tryGet<T>();
			if (!service)
			{
				LOG_FATAL("Services", "Required service {} has not been provided", typeid( T ).name());
				std::abort();
			}
			return *service;
		}

	private:
		std::unordered_map<std::type_index, void*> m_services;
	};
}
