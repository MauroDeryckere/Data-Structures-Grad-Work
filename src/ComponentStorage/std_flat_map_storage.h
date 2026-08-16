#ifndef MAU_STD_FLAT_MAP_STORAGE_H
#define MAU_STD_FLAT_MAP_STORAGE_H

#include "component_storage.h"

#include <ranges>

// only defines StdFlatMapStorage when the standard library actually ships std::flat_map
// (libstdc++ 15+, libc++ 20+, MSVC STL 19.51+).
#if __has_include(<flat_map>)
	#include <flat_map>
	#ifdef __cpp_lib_flat_map
		#define MAU_HAS_STD_FLAT_MAP 1
	#endif
#endif

#ifdef MAU_HAS_STD_FLAT_MAP

namespace Mau
{
	template <typename Val>
	class StdFlatMapStorage final
	{
	public:
		Val& Add(Entity e, Val const& value) noexcept
		{
			return m_Storage.emplace(e, value).first->second;
		}

		Val& AddNew(Entity e, Val const& value) noexcept
		{
			return Add(e, value);
		}

		bool Remove(Entity e) noexcept
		{
			return m_Storage.erase(e) > 0;
		}

		[[nodiscard]] bool Has(Entity e) const noexcept
		{
			return m_Storage.find(e) != m_Storage.end();
		}

		[[nodiscard]] Val* Find(Entity e) noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != m_Storage.end() ? &it->second : nullptr;
		}
		[[nodiscard]] Val const* Find(Entity e) const noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != m_Storage.end() ? &it->second : nullptr;
		}

		[[nodiscard]] size_t size() const noexcept { return m_Storage.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_Storage.empty(); }
		void clear() noexcept { m_Storage.clear(); }

		// Same extract()/replace() pattern as FlatMapStorage::Reserve - std::flat_map exposes the identical P0429 pair, its reference implementation.
		void Reserve(size_t n) noexcept
		{
			auto containers{ std::move(m_Storage).extract() };
			containers.keys.reserve(n);
			containers.values.reserve(n);
			m_Storage.replace(std::move(containers.keys), std::move(containers.values));
		}

		[[nodiscard]] auto Values() noexcept { return m_Storage | std::views::values; }
		[[nodiscard]] auto Values() const noexcept { return m_Storage | std::views::values; }

	private:
		std::flat_map<Entity, Val> m_Storage;
	};
}

#endif // MAU_HAS_STD_FLAT_MAP

#endif
