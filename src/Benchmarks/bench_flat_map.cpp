#include "bench_flat_map.h"
#include "../benchmark.h"

#include <SG14/flat_map.h>

namespace Mau
{
	stdext::flat_map<Entity, ComponentSmall> g_TestFlatMap;

	void RegisterFlatMapBenchmarks()
	{
		auto& reg = Mau::BenchmarkRegistry::GetInstance();
		reg.Register("Flat Map Emplace", "Map Emplace", BenchmarkFlatMapEmplace, TEST_ITERATIONS);
		reg.Register("Flat Map Iterate", "Map Iterate", BenchmarkFlatMapIterate, TEST_ITERATIONS);
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

	void BenchmarkFlatMapEmplace()
	{
		g_TestFlatMap.clear();

		for (uint32_t i{ 0 }; i < TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestFlatMap.emplace(i, value);
		}
	}
}