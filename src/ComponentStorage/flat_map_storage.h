#ifndef MAU_FLAT_MAP_STORAGE_H
#define MAU_FLAT_MAP_STORAGE_H

#include "component_storage.h"
#include <SG14/flat_map.h>

#include <ranges>

namespace Mau
{
	// SG14 reference flat_map
	template <typename Val>
	class FlatMapStorage final
	{
	public:
		Val& Add(Entity e, Val const& value) noexcept
		{
			// emplace() is a no-op (returns the existing element) if e is already
			// present, so this is safe to call regardless of prior presence.
			return m_Storage.emplace(e, value).first->second;
		}

		// No cheaper native op to route to
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
			return m_Storage.find(e) != end(m_Storage);
		}

		[[nodiscard]] Val* Find(Entity e) noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != end(m_Storage) ? &it->second : nullptr;
		}
		[[nodiscard]] Val const* Find(Entity e) const noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != end(m_Storage) ? &it->second : nullptr;
		}

		[[nodiscard]] size_t size() const noexcept { return m_Storage.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_Storage.empty(); }
		void clear() noexcept { m_Storage.clear(); }

		// SG14's flat_map has no reserve() of its own, but it exposes the P0429
		// extract()/replace() pair specifically for bulk-manipulating the underlying key/value containers
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
		stdext::flat_map<Entity, Val> m_Storage;
	};
}

#endif
