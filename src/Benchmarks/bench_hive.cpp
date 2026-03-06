#include "bench_hive.h"
#include "../benchmark.h"

#include "plf_hive/plf_hive.h"
#include <unordered_map>

namespace Mau
{
	struct HiveComponent
	{
		Entity entity;
		ComponentSmall value;
	};

	plf::hive<HiveComponent> g_TestHiveCombined;
	plf::hive<ComponentSmall> g_TestHiveValueOnly;

	// Side map for key-based lookup/erase on hive (which has no native key access)
	std::unordered_map<Entity, plf::hive<HiveComponent>::iterator> g_HiveLookupMap;

	void RegisterHiveBenchmarks()
	{
		auto& reg{ Mau::BenchmarkRegistry::GetInstance() };

		reg.Register("Hive Combined Emplace", "Emplace", BenchmarkHiveCombinedEmplace, TEST_ITERATIONS, BenchmarkHiveCombinedEmplaceSetup);
		reg.Register("Hive Combined Iterate", "Iterate", BenchmarkHiveCombinedIterate, TEST_ITERATIONS, BenchmarkHiveCombinedIterateSetup);
		reg.Register("Hive Combined Lookup (side map)", "Lookup", BenchmarkHiveCombinedLookup, TEST_ITERATIONS, BenchmarkHiveCombinedLookupSetup);
		reg.Register("Hive Combined Erase (side map)", "Erase", BenchmarkHiveCombinedErase, TEST_ITERATIONS, BenchmarkHiveCombinedEraseSetup);

		reg.Register("Hive Value Only Emplace", "Emplace", BenchmarkHiveValueOnlyEmplace, TEST_ITERATIONS, BenchmarkHiveValueOnlyEmplaceSetup);
		reg.Register("Hive Value Only Iterate", "Iterate", BenchmarkHiveValueOnlyIterate, TEST_ITERATIONS, BenchmarkHiveValueOnlyIterateSetup);
	}

	// --- Combined ---

	void BenchmarkHiveCombinedEmplaceSetup()
	{
		g_TestHiveCombined.clear();
	}

	void BenchmarkHiveCombinedEmplace()
	{
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			g_TestHiveCombined.emplace(i, Mau::GenerateValue(i));
		}
		ClobberMemory();
	}

	void BenchmarkHiveCombinedIterateSetup()
	{
		BenchmarkHiveCombinedEmplaceSetup();
		BenchmarkHiveCombinedEmplace();
	}

	void BenchmarkHiveCombinedIterate()
	{
		float sum{ 0.0f };
		for (auto const& item : g_TestHiveCombined)
		{
			sum += item.value * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	static void FillHiveCombinedWithLookupMap()
	{
		g_TestHiveCombined.clear();
		g_HiveLookupMap.clear();

		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			auto it{ g_TestHiveCombined.emplace(i, Mau::GenerateValue(i)) };
			g_HiveLookupMap.emplace(i, it);
		}
	}

	void BenchmarkHiveCombinedLookupSetup()
	{
		FillHiveCombinedWithLookupMap();
	}

	void BenchmarkHiveCombinedLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_HiveLookupMap.find(e) };
			if (it == g_HiveLookupMap.end())
			{
				continue;
			}

			sum += it->second->value * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	void BenchmarkHiveCombinedEraseSetup()
	{
		FillHiveCombinedWithLookupMap();
	}

	void BenchmarkHiveCombinedErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			auto mapIt{ g_HiveLookupMap.find(e) };
			if (mapIt == g_HiveLookupMap.end())
			{
				continue;
			}

			g_TestHiveCombined.erase(mapIt->second);
			g_HiveLookupMap.erase(mapIt);
		}
		ClobberMemory();
	}

	// --- Value Only ---

	void BenchmarkHiveValueOnlyEmplaceSetup()
	{
		g_TestHiveValueOnly.clear();
	}

	void BenchmarkHiveValueOnlyEmplace()
	{
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			g_TestHiveValueOnly.emplace(Mau::GenerateValue(i));
		}
		ClobberMemory();
	}

	void BenchmarkHiveValueOnlyIterateSetup()
	{
		BenchmarkHiveValueOnlyEmplaceSetup();
		BenchmarkHiveValueOnlyEmplace();
	}

	void BenchmarkHiveValueOnlyIterate()
	{
		float sum{ 0.0f };

		for (auto const& value : g_TestHiveValueOnly)
		{
			sum += value * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}
}
