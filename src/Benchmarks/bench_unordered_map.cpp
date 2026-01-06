#include "bench_unordered_map.h"
#include "../benchmark.h"

#include <unordered_map>

namespace Mau
{
	std::unordered_map<int, float> g_TestUnorderedMap;


	void RegisterUnorderedMapBenchmarks()
	{

	}

	void BenchmarkUnorderedMapIterate()
	{
		float sum{ 0.0f };

		for (auto& item : g_TestUnorderedMap)
		{
			sum += item.second * 2.0f;
			DO_NOT_OPTIMIZE(sum);
		}
		CLOBBER_MEMORY();
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