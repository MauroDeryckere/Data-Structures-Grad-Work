#ifndef MAU_UNORDERED_MAP_STORAGE_H
#define MAU_UNORDERED_MAP_STORAGE_H

#include "component_storage.h"

#include <unordered_map>
#include <ranges>

namespace Mau
{
	template <typename Val>
	class UnorderedMapStorage final
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

		void Reserve(size_t n) noexcept { m_Storage.reserve(n); }

		[[nodiscard]] auto Values() noexcept { return m_Storage | std::views::values; }
		[[nodiscard]] auto Values() const noexcept { return m_Storage | std::views::values; }

	private:
		std::unordered_map<Entity, Val> m_Storage;
	};
}

#endif
