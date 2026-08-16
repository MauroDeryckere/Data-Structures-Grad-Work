#ifndef MAU_HIVE_STORAGE_H
#define MAU_HIVE_STORAGE_H

#include "component_storage.h"
#include "plf_hive/plf_hive.h"

#include <vector>
#include <memory>
#include <ranges>

namespace Mau
{
	// Unlike every other backend, plf::hive has no native key-based access at all, it's
	// a pure sequence container. This wrapper needs an internal side-index (Entity ->
	// element location) to satisfy the same interface the other five backends get for
	// free from their own key-based APIs.
	//
	// The side-index is a dense std::vector<Val*> indexed directly by Entity (mirroring
	// sparse_set's own sparse-array design), not a std::unordered_map or a vector of hive
	// iterators. Two reasons:
	//  - Dense array vs. hash map: every benchmark here uses small, dense entity ID ranges,
	//    where a direct-indexed array gives true O(1) worst-case lookup with no hashing and
	//    better cache locality. Wrong choice for a sparse entity ID space, where it wastes
	//    memory a hash map wouldn't, worth revisiting once sparse-iteration workloads exist.
	//  - Raw pointer vs. iterator: plf::hive::iterator holds three pointers internally
	//    (group_pointer, element_pointer, skipfield_pointer - see plf_hive.h), 24 bytes on a
	//    64-bit build, vs. 8 for a plain Val*. hive exposes get_iterator(pointer) noexcept
	//    specifically to convert a raw element pointer back into a full iterator on demand,
	//    so the side-index only needs to pay the full iterator's size at the one call site
	//    that actually needs it (Remove), not for every stored entity.
	template <typename Val>
	class HiveStorage final
	{
	public:
		Val& Add(Entity e, Val const& value) noexcept
		{
			if (Has(e))
			{
				return *m_SparseIndex[e];
			}

			return AddNew(e, value);
		}

		// Precondition: e is not already present. Skips the presence check Add() has
		Val& AddNew(Entity e, Val const& value) noexcept
		{
			if (e >= m_SparseIndex.size())
			{
				m_SparseIndex.resize(e + 1, nullptr);
			}

			auto const it{ m_Storage.emplace(value) };
			m_SparseIndex[e] = std::addressof(*it);
			return *it;
		}

		bool Remove(Entity e) noexcept
		{
			if (!Has(e))
			{
				return false;
			}

			m_Storage.erase(m_Storage.get_iterator(m_SparseIndex[e]));
			m_SparseIndex[e] = nullptr;
			return true;
		}

		[[nodiscard]] bool Has(Entity e) const noexcept
		{
			return e < m_SparseIndex.size() && m_SparseIndex[e] != nullptr;
		}

		[[nodiscard]] Val* Find(Entity e) noexcept
		{
			return Has(e) ? m_SparseIndex[e] : nullptr;
		}
		[[nodiscard]] Val const* Find(Entity e) const noexcept
		{
			return Has(e) ? m_SparseIndex[e] : nullptr;
		}

		[[nodiscard]] size_t size() const noexcept { return m_Storage.size(); }
		[[nodiscard]] bool empty() const noexcept { return m_Storage.empty(); }
		void clear() noexcept { m_Storage.clear(); m_SparseIndex.clear(); }

		// hive allocates in fixed-capacity blocks rather than one contiguous buffer, so
		// reserving upfront avoids block-by-block growth during bulk inserts
		void Reserve(size_t n) noexcept
		{
			m_Storage.reserve(n);
			m_SparseIndex.reserve(n);
		}

		[[nodiscard]] auto Values() noexcept { return std::ranges::subrange(m_Storage.begin(), m_Storage.end()); }
		[[nodiscard]] auto Values() const noexcept { return std::ranges::subrange(m_Storage.begin(), m_Storage.end()); }

	private:
		plf::hive<Val> m_Storage;
		std::vector<Val*> m_SparseIndex;
	};
}

#endif
