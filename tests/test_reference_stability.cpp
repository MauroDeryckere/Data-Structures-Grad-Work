#include "doctest/doctest.h"

#include "../src/ComponentStorage/component_storage.h"
#include "../src/ComponentStorage/sparse_set_storage.h"
#include "../src/ComponentStorage/flat_map_storage.h"
#include "../src/ComponentStorage/std_flat_map_storage.h"
#include "../src/ComponentStorage/hive_storage.h"
#include "../src/ComponentStorage/map_storage.h"
#include "../src/ComponentStorage/unordered_map_storage.h"

namespace
{
	// Direct empirical grounding for H1.1/H1.2's reference/iterator-stability claims:
	// does a pointer obtained before unrelated churn elsewhere in the storage still
	// refer to the same object afterward? Compares *addresses* only (before == after)
	// and never dereferences `before` after the churn - a dangling pointer's address
	// is safe to compare, dereferencing it would not be.
	template <typename StorageType>
		requires Mau::ComponentStorage<StorageType, float>
	bool ProbeReferenceStability()
	{
		using Mau::Entity;

		StorageType storage;
		for (Entity e{ 0 }; e < 8; ++e)
		{
			storage.Add(e, static_cast<float>(e));
		}

		float const* before{ storage.Find(0) };
		REQUIRE(before != nullptr);

		// Unrelated churn: remove several other live entities, then add enough new
		// ones to force reallocation/rehashing on any backend that would do so.
		for (Entity e{ 1 }; e < 8; ++e)
		{
			storage.Remove(e);
		}
		for (Entity e{ 100 }; e < 200; ++e)
		{
			storage.Add(e, static_cast<float>(e));
		}

		float const* after{ storage.Find(0) };
		REQUIRE(after != nullptr);

		return before == after;
	}
}

TEST_CASE("Reference stability: sparse_set (expected UNSTABLE - contiguous, swap-and-pop erase)")
{
	CHECK_FALSE(ProbeReferenceStability<Mau::SparseSetStorage<float>>());
}

TEST_CASE("Reference stability: SG14 flat_map (expected UNSTABLE - sorted std::vector)")
{
	CHECK_FALSE(ProbeReferenceStability<Mau::FlatMapStorage<float>>());
}

#ifdef MAU_HAS_STD_FLAT_MAP
TEST_CASE("Reference stability: std::flat_map (expected UNSTABLE - sorted std::vector)")
{
	CHECK_FALSE(ProbeReferenceStability<Mau::StdFlatMapStorage<float>>());
}
#endif

TEST_CASE("Reference stability: std::map (expected STABLE - node-based)")
{
	CHECK(ProbeReferenceStability<Mau::MapStorage<float>>());
}

TEST_CASE("Reference stability: std::unordered_map (expected STABLE - refs survive rehash, only iterators don't)")
{
	CHECK(ProbeReferenceStability<Mau::UnorderedMapStorage<float>>());
}

TEST_CASE("Reference stability: plf::hive (expected STABLE - the container's core design guarantee)")
{
	CHECK(ProbeReferenceStability<Mau::HiveStorage<float>>());
}
