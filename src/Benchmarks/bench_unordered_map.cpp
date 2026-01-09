#include "bench_unordered_map.h"
#include "../benchmark.h"

#include <unordered_map>

namespace Mau
{
	std::unordered_map<Entity, ComponentSmall> g_TestUnorderedMap;


	void RegisterUnorderedMapBenchmarks()
	{
		auto& benchmarkReg{ Mau::BenchmarkRegistry::GetInstance() };
		benchmarkReg.Register("Unordered Map Emplace", "Map Emplace", BenchmarkUnorderedMapEmplace, TEST_ITERATIONS);
		benchmarkReg.Register("Unordered Map Iterate", "Map Iterate", BenchmarkUnorderedMapIterate, TEST_ITERATIONS);
	}

	void BenchmarkUnorderedMapIterate()
	{
		float sum{ 0.0f };

		for (auto& item : g_TestUnorderedMap)
		{
			sum += item.second * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkUnorderedMapEmplace()
	{
		g_TestUnorderedMap.clear();

		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestUnorderedMap.emplace(i, value);
		}
	}
}