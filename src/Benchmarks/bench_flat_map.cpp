#include "bench_flat_map.h"
#include "../benchmark.h"

#include <SG14/flat_map.h>

namespace
{
	stdext::flat_map<int, float> g_TestFlatMap;
}

namespace Mau
{
	void RegisterFlatMapBenchmarks()
	{
		auto& reg = Mau::BenchmarkRegistry::GetInstance();
		reg.Register("Flat Map Emplace", "Map Emplace", BenchmarkFlatMapEmplace, 10);
		reg.Register("Flat Map Iterate", "Map Iterate", BenchmarkFlatMapIterate, 10);
	}

	void BenchmarkFlatMapIterate()
	{
		float sum{ 0.0f };

		for (auto const& item : g_TestFlatMap)
		{
			sum += item.second * 2.0f;
		}

		DO_NOT_OPTIMIZE(sum);
		CLOBBER_MEMORY();
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