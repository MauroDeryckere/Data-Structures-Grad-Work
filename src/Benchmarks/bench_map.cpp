#include "bench_map.h"
#include "../benchmark.h"

#include <map>

namespace Mau
{
	std::map<Entity, ComponentSmall> g_TestMap;

	void RegisterMapBenchmarks()
	{
		auto& benchmarkReg{ Mau::BenchmarkRegistry::GetInstance() };
		benchmarkReg.Register("Map Emplace", "Map Emplace", BenchmarkMapEmplace, TEST_ITERATIONS);
		benchmarkReg.Register("Map Iterate", "Map Iterate", BenchmarkMapIterate, TEST_ITERATIONS);
	}

	void BenchmarkMapIterate()
	{
		float sum{ 0.0f };

		for (auto& item : g_TestMap)
		{
			sum += item.second * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkMapEmplace()
	{
		g_TestMap.clear();

		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestMap.emplace(i, value);
		}
	}
}
