#include "bench_flat_map.h"
#include "../benchmark.h"

#include <SG14/flat_map.h>

namespace Mau
{
	stdext::flat_map<Entity, ComponentSmall> g_TestFlatMap;

	void RegisterFlatMapBenchmarks()
	{
		auto& reg = Mau::BenchmarkRegistry::GetInstance();

		reg.Register("Flat Map Emplace", "Map Emplace", BenchmarkFlatMapEmplace, TEST_ITERATIONS, BenchmarkFlatMapEmplaceSetup);
		reg.Register("Flat Map Iterate", "Map Iterate", BenchmarkFlatMapIterate, TEST_ITERATIONS, BenchmarkFlatMapIterateSetup);
		reg.Register("Flat Map Lookup", "Map Lookup", BenchmarkFlatMapLookup, TEST_ITERATIONS, BenchmarkFlatMapLookupSetup);
		reg.Register("Flat Map Erase", "Map Erase", BenchmarkFlatMapErase, TEST_ITERATIONS, BenchmarkFlatMapEraseSetup);
	}

	void BenchmarkFlatMapEmplaceSetup()
	{
		g_TestFlatMap.clear();
	}

	void BenchmarkFlatMapEmplace()
	{
		for (uint32_t i{ 0 }; i < TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestFlatMap.emplace(i, value);
		}
		ClobberMemory();
	}

	void BenchmarkFlatMapIterateSetup()
	{
		BenchmarkFlatMapEmplaceSetup();
		BenchmarkFlatMapEmplace();
	}

	void BenchmarkFlatMapIterate()
	{
		float sum{ 0.0f };

		for (auto const& item : g_TestFlatMap)
		{
			sum += item.second * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkFlatMapLookupSetup()
	{
		BenchmarkFlatMapEmplaceSetup();
		BenchmarkFlatMapEmplace();
	}

	void BenchmarkFlatMapLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_TestFlatMap.find(e) };
			if (it == end(g_TestFlatMap))
			{
				continue;
			}

			sum += it->second * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	void BenchmarkFlatMapEraseSetup()
	{
		BenchmarkFlatMapEmplaceSetup();
		BenchmarkFlatMapEmplace();
	}

	void BenchmarkFlatMapErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			g_TestFlatMap.erase(e);
		}
		ClobberMemory();
	}
}