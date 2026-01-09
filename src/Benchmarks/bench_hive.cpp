#include "bench_hive.h"
#include "../benchmark.h"

#include "plf_hive/plf_hive.h"

// For hive we might need to add a map to do lookups likely not a good idea to benchmark hive for ECS.

namespace Mau
{
	struct HiveComponent 
	{
		Entity entity;
		ComponentSmall value;
	};

	plf::hive<HiveComponent> g_TestHiveCombined;
	plf::hive<ComponentSmall> g_TestHiveValueOnly;


	void RegisterHiveBenchmarks()
	{
		auto& reg = Mau::BenchmarkRegistry::GetInstance();

		// Combined
		reg.Register("Hive Combined Emplace", "Hive Emplace", BenchmarkHiveCombinedEmplace, 10);
		reg.Register("Hive Combined Iterate", "Hive Iterate", BenchmarkHiveCombinedIterate, 10);

		// Value Only
		reg.Register("Hive Value Only Emplace", "Hive Emplace", BenchmarkHiveValueOnlyEmplace, 10);
		reg.Register("Hive Value Only Iterate", "Hive Iterate", BenchmarkHiveValueOnlyIterate, 10);
	}

	void BenchmarkHiveCombinedIterate()
	{
		float sum{ 0.0f };
		for (auto& item : g_TestHiveCombined)
		{
			sum += item.value * 2.0f;
			DO_NOT_OPTIMIZE(sum);
		}
		CLOBBER_MEMORY();
	}

	void BenchmarkHiveCombinedEmplace()
	{
		g_TestHiveCombined.clear();
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			g_TestHiveCombined.emplace(i, Mau::GenerateValue(i));
		}
	}

	void BenchmarkHiveValueOnlyIterate()
	{
		float sum{ 0.0f };
		for (auto& value : g_TestHiveValueOnly)
		{
			sum += value * 2.0f;
			DO_NOT_OPTIMIZE(sum);
		}
		CLOBBER_MEMORY();
	}

	void BenchmarkHiveValueOnlyEmplace()
	{
		g_TestHiveValueOnly.clear();
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			g_TestHiveValueOnly.emplace(Mau::GenerateValue(i));
		}
	}
}
