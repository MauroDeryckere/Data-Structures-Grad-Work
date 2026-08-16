#ifndef MAU_SPARSE_SET_STORAGE_H
#define MAU_SPARSE_SET_STORAGE_H

#include "component_storage.h"
#include "SparseSet/SparseSet.h"

#include <ranges>

namespace Mau
{
	template <typename Val>
	class SparseSetStorage final
	{
	public:
		Val& Add(Entity e, Val const& value) noexcept
		{
			return m_Storage.get_or_emplace(e, value);
		}

		//the fast path this wrapper's Add() otherwise pays for with an extra contains() lookup.
		Val& AddNew(Entity e, Val const& value) noexcept
		{
			return m_Storage.emplace(e, value);
		}

		bool Remove(Entity e) noexcept
		{
			return m_Storage.remove(e);
		}

		[[nodiscard]] bool Has(Entity e) const noexcept
		{
			return m_Storage.contains(e);
		}

		[[nodiscard]] Val* Find(Entity e) noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != m_Storage.end() ? &*it : nullptr;
		}
		[[nodiscard]] Val const* Find(Entity e) const noexcept
		{
			auto const it{ m_Storage.find(e) };
			return it != m_Storage.end() ? &*it : nullptr;
		}

		[[nodiscard]] size_t size() const noexcept { return m_Storage.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_Storage.empty(); }
		void clear() noexcept { m_Storage.clear(); }
		void Reserve(size_t n) noexcept { m_Storage.reserve(static_cast<Entity>(n)); }

		[[nodiscard]] auto Values() noexcept { return std::ranges::subrange(m_Storage.begin(), m_Storage.end()); }
		[[nodiscard]] auto Values() const noexcept { return std::ranges::subrange(m_Storage.begin(), m_Storage.end()); }

	private:
		sparse_set<Val, Entity> m_Storage;
	};
}

#endif
