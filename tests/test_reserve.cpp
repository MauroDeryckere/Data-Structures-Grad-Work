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
	template <typename StorageType>
		requires Mau::ComponentStorage<StorageType, float>
	void RunReserveChecks()
	{
		using Mau::Entity;

		// Reserve() on a freshly-constructed, empty storage doesn't crash, and the
		// storage is still fully functional afterward.
		{
			StorageType storage;
			storage.Reserve(64);
			CHECK(storage.empty());
			storage.AddNew(0, 1.0f);
			CHECK(storage.Has(0));
			CHECK(storage.size() == size_t{ 1 });
		}

		// Reserve(0) edge case doesn't crash and doesn't break subsequent use.
		{
			StorageType storage;
			storage.Reserve(0);
			storage.AddNew(0, 5.0f);
			float* found{ storage.Find(0) };
			REQUIRE(found != nullptr);
			CHECK(*found == doctest::Approx(5.0f));
		}

		// Reserve(n) then bulk-insert exactly n known-fresh entities via AddNew -
		// every one of them ends up correct.
		{
			StorageType storage;
			storage.Reserve(50);
			for (Entity e{ 0 }; e < 50; ++e)
			{
				storage.AddNew(e, static_cast<float>(e));
			}
			CHECK(storage.size() == size_t{ 50 });

			for (Entity e{ 0 }; e < 50; ++e)
			{
				CHECK(storage.Has(e));
				float* found{ storage.Find(e) };
				REQUIRE(found != nullptr);
				CHECK(*found == doctest::Approx(static_cast<float>(e)));
			}

			int count{ 0 };
			float sum{ 0.0f };
			for (float const& v : storage.Values())
			{
				++count;
				sum += v;
			}
			CHECK(count == 50);
			CHECK(sum == doctest::Approx(49.0f * 50.0f / 2.0f)); // sum of 0..49
		}

		// The critical case: Reserve() called mid-use, after entities are already
		// present - not just on a freshly-constructed storage.
		{
			StorageType storage;
			for (Entity e{ 0 }; e < 5; ++e)
			{
				storage.AddNew(e, static_cast<float>(e) * 10.0f);
			}
			REQUIRE(storage.size() == size_t{ 5 });

			storage.Reserve(200);

			// Every entity added before Reserve() must have survived, unchanged.
			CHECK(storage.size() == size_t{ 5 });
			for (Entity e{ 0 }; e < 5; ++e)
			{
				CHECK(storage.Has(e));
				float* found{ storage.Find(e) };
				REQUIRE(found != nullptr);
				CHECK(*found == doctest::Approx(static_cast<float>(e) * 10.0f));
			}

			int countAfterReserve{ 0 };
			for ([[maybe_unused]] float const& v : storage.Values()) { ++countAfterReserve; }
			CHECK(countAfterReserve == 5);

			// And the storage is still fully usable afterward.
			for (Entity e{ 100 }; e < 150; ++e)
			{
				storage.AddNew(e, static_cast<float>(e));
			}
			CHECK(storage.size() == size_t{ 55 });
			for (Entity e{ 100 }; e < 150; ++e)
			{
				CHECK(storage.Has(e));
			}
		}
	}
}

TEST_CASE_TEMPLATE("Reserve: correctness", StorageType,
	Mau::SparseSetStorage<float>,
	Mau::FlatMapStorage<float>,
	Mau::MapStorage<float>,
	Mau::UnorderedMapStorage<float>,
	Mau::HiveStorage<float>)
{
	RunReserveChecks<StorageType>();
}

#ifdef MAU_HAS_STD_FLAT_MAP
TEST_CASE_TEMPLATE("Reserve: correctness (std::flat_map)", StorageType,
	Mau::StdFlatMapStorage<float>)
{
	RunReserveChecks<StorageType>();
}
#endif

TEST_CASE("Reserve: does not break plf::hive's reference stability guarantee")
{
	using Mau::Entity;

	Mau::HiveStorage<float> storage;
	for (Entity e{ 0 }; e < 8; ++e)
	{
		storage.AddNew(e, static_cast<float>(e));
	}

	float const* before{ storage.Find(0) };
	REQUIRE(before != nullptr);

	storage.Reserve(500); // touches block allocation directly

	float const* after{ storage.Find(0) };
	REQUIRE(after != nullptr);
	CHECK(before == after);
	CHECK(*after == doctest::Approx(0.0f));
}

TEST_CASE("Reserve: unordered_map's rehash-triggering reserve() preserves reference stability")
{
	using Mau::Entity;

	Mau::UnorderedMapStorage<float> storage;
	for (Entity e{ 0 }; e < 8; ++e)
	{
		storage.AddNew(e, static_cast<float>(e));
	}

	float const* before{ storage.Find(0) };
	REQUIRE(before != nullptr);

	// Standard guarantee: rehashing (which a large reserve() forces) invalidates
	// iterators, but never references/pointers to existing elements.
	storage.Reserve(500);

	float const* after{ storage.Find(0) };
	REQUIRE(after != nullptr);
	CHECK(before == after);
	CHECK(*after == doctest::Approx(0.0f));
}
