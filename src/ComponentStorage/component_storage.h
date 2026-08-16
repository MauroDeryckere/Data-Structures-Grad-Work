#ifndef MAU_COMPONENT_STORAGE_H
#define MAU_COMPONENT_STORAGE_H

#include <cstddef>
#include <concepts>

#include "../benchmark.h"

namespace Mau
{
	// Minimal common interface every ECS component storage backend must satisfy, so
	// callers can swap backends without changing usage code.
	// Add() - get-or-insert, defined behavior on a duplicate key
	// AddNew() is the fast-path counterpart with the precondition "e is not already present,"
	// they're only distinct*operations where the container can actually skip its own uniqueness check (sparse_set, hive)
	template <typename S, typename Val>
	concept ComponentStorage = requires(S s, S const cs, Entity e, Val v)
	{
		{ s.Add(e, v) } -> std::same_as<Val&>;
		{ s.AddNew(e, v) } -> std::same_as<Val&>;
		{ s.Remove(e) } -> std::same_as<bool>;
		{ cs.Has(e) } -> std::same_as<bool>;
		{ s.Find(e) } -> std::same_as<Val*>;
		{ cs.Find(e) } -> std::same_as<Val const*>;
		{ cs.size() } -> std::same_as<size_t>;
		{ cs.empty() } -> std::same_as<bool>;
		{ s.clear() };
		{ s.Reserve(size_t{}) };

		// Values() gives every backend the same iteration shape (a range of Val&),
		// even though each backend's native iterator dereferences to something
		// different underneath (sparse_set: Val directly; the map-likes and hive:
		// a pair<Entity, Val> that Values() adapts via std::views::values).
		s.Values(); cs.Values();
	};
}

#endif
