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
	// RQ1's methodology list, directly: create/destroy (construction, clear()),
	// add component, remove component, lookup, iteration - run identically against
	// every backend via the shared ComponentStorage interface.
	template <typename StorageType>
		requires Mau::ComponentStorage<StorageType, float>
	void RunBasicOperationsChecks()
	{
		using Mau::Entity;

		StorageType storage;
		CHECK(storage.empty());
		CHECK(storage.size() == size_t{ 0 });
		CHECK_FALSE(storage.Has(1));
		CHECK(storage.Find(1) == nullptr);

		float& added{ storage.Add(1, 3.5f) };
		CHECK(added == doctest::Approx(3.5f));
		CHECK(storage.Has(1));
		CHECK(storage.size() == size_t{ 1 });
		CHECK_FALSE(storage.empty());

		storage.Add(2, 7.0f);
		storage.Add(3, 9.0f);
		CHECK(storage.size() == size_t{ 3 });

		CHECK(storage.Has(2));
		CHECK_FALSE(storage.Has(42));
		float* found{ storage.Find(2) };
		REQUIRE(found != nullptr);
		CHECK(*found == doctest::Approx(7.0f));
		CHECK(storage.Find(42) == nullptr);

		{
			int count{ 0 };
			float sum{ 0.0f };
			for (float const& v : storage.Values())
			{
				++count;
				sum += v;
			}
			CHECK(count == 3);
			CHECK(sum == doctest::Approx(3.5f + 7.0f + 9.0f));
		}

		CHECK(storage.Remove(2));
		CHECK_FALSE(storage.Has(2));
		CHECK(storage.size() == size_t{ 2 });

		// Removing an already-absent entity is safe (defined, non-UB behavior every
		// wrapper standardizes on, even where a backend's raw API would assert/UB).
		CHECK_FALSE(storage.Remove(2));
		CHECK_FALSE(storage.Remove(999));
		CHECK(storage.size() == size_t{ 2 });

		{
			int count{ 0 };
			for ([[maybe_unused]] float const& v : storage.Values()) { ++count; }
			CHECK(count == 2);
		}

		// Add on an already-present entity is a safe no-op (keeps the existing value)
		// across every backend - not all backends' native APIs give you this for free
		// (sparse_set's raw emplace() is UB on a duplicate key).
		storage.Add(1, 100.0f);
		CHECK(storage.size() == size_t{ 2 });
		float* stillOriginal{ storage.Find(1) };
		REQUIRE(stillOriginal != nullptr);
		CHECK(*stillOriginal == doctest::Approx(3.5f));

		// AddNew: fast path for a guaranteed-fresh key (precondition: not already
		// present) - behaves identically to Add() in that case, just without paying
		// for a presence check on backends that can skip one.
		float& addedNew{ storage.AddNew(50, 42.0f) };
		CHECK(addedNew == doctest::Approx(42.0f));
		CHECK(storage.Has(50));
		CHECK(storage.size() == size_t{ 3 });
		float* foundNew{ storage.Find(50) };
		REQUIRE(foundNew != nullptr);
		CHECK(*foundNew == doctest::Approx(42.0f));

		storage.clear();
		CHECK(storage.empty());
		CHECK(storage.size() == size_t{ 0 });
		CHECK_FALSE(storage.Has(1));
	}
}

TEST_CASE_TEMPLATE("Component storage: basic operations", StorageType,
	Mau::SparseSetStorage<float>,
	Mau::FlatMapStorage<float>,
	Mau::MapStorage<float>,
	Mau::UnorderedMapStorage<float>,
	Mau::HiveStorage<float>)
{
	RunBasicOperationsChecks<StorageType>();
}

#ifdef MAU_HAS_STD_FLAT_MAP
TEST_CASE_TEMPLATE("Component storage: basic operations (std::flat_map)", StorageType,
	Mau::StdFlatMapStorage<float>)
{
	RunBasicOperationsChecks<StorageType>();
}
#endif
