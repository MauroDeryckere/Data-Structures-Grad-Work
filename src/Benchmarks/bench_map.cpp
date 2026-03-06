#include "bench_map.h"
#include "../benchmark.h"

#include <map>

namespace Mau
{
	std::map<Entity, ComponentSmall> g_TestMap;

	void RegisterMapBenchmarks()
	{
		auto& reg{ Mau::BenchmarkRegistry::GetInstance() };

		reg.Register("Map Emplace", "Emplace", BenchmarkMapEmplace, TEST_ITERATIONS, BenchmarkMapEmplaceSetup);
		reg.Register("Map Iterate", "Iterate", BenchmarkMapIterate, TEST_ITERATIONS, BenchmarkMapIterateSetup);
		reg.Register("Map Lookup", "Lookup", BenchmarkMapLookup, TEST_ITERATIONS, BenchmarkMapLookupSetup);
		reg.Register("Map Erase", "Erase", BenchmarkMapErase, TEST_ITERATIONS, BenchmarkMapEraseSetup);
	}

	void BenchmarkMapEmplaceSetup()
	{
		g_TestMap.clear();
	}

	void BenchmarkMapEmplace()
	{
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestMap.emplace(i, value);
		}
		ClobberMemory();
	}

	void BenchmarkMapIterateSetup()
	{
		BenchmarkMapEmplaceSetup();
		BenchmarkMapEmplace();
	}

	void BenchmarkMapIterate()
	{
		float sum{ 0.0f };

		for (auto const& item : g_TestMap)
		{
			sum += item.second * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkMapLookupSetup()
	{
		BenchmarkMapEmplaceSetup();
		BenchmarkMapEmplace();
	}

	void BenchmarkMapLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_TestMap.find(e) };
			if (it == g_TestMap.end())
			{
				continue;
			}

			sum += it->second * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	void BenchmarkMapEraseSetup()
	{
		BenchmarkMapEmplaceSetup();
		BenchmarkMapEmplace();
	}

	void BenchmarkMapErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			g_TestMap.erase(e);
		}
		ClobberMemory();
	}
}
