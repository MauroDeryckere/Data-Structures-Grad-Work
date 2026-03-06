#include "bench_unordered_map.h"
#include "../benchmark.h"

#include <unordered_map>

namespace Mau
{
	std::unordered_map<Entity, ComponentSmall> g_TestUnorderedMap;

	void RegisterUnorderedMapBenchmarks()
	{
		auto& reg{ Mau::BenchmarkRegistry::GetInstance() };

		reg.Register("Unordered Map Emplace", "Emplace", BenchmarkUnorderedMapEmplace, TEST_ITERATIONS, BenchmarkUnorderedMapEmplaceSetup);
		reg.Register("Unordered Map Iterate", "Iterate", BenchmarkUnorderedMapIterate, TEST_ITERATIONS, BenchmarkUnorderedMapIterateSetup);
		reg.Register("Unordered Map Lookup", "Lookup", BenchmarkUnorderedMapLookup, TEST_ITERATIONS, BenchmarkUnorderedMapLookupSetup);
		reg.Register("Unordered Map Erase", "Erase", BenchmarkUnorderedMapErase, TEST_ITERATIONS, BenchmarkUnorderedMapEraseSetup);
	}

	void BenchmarkUnorderedMapEmplaceSetup()
	{
		g_TestUnorderedMap.clear();
	}

	void BenchmarkUnorderedMapEmplace()
	{
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestUnorderedMap.emplace(i, value);
		}
		ClobberMemory();
	}

	void BenchmarkUnorderedMapIterateSetup()
	{
		BenchmarkUnorderedMapEmplaceSetup();
		BenchmarkUnorderedMapEmplace();
	}

	void BenchmarkUnorderedMapIterate()
	{
		float sum{ 0.0f };

		for (auto const& item : g_TestUnorderedMap)
		{
			sum += item.second * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkUnorderedMapLookupSetup()
	{
		BenchmarkUnorderedMapEmplaceSetup();
		BenchmarkUnorderedMapEmplace();
	}

	void BenchmarkUnorderedMapLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_TestUnorderedMap.find(e) };
			if (it == g_TestUnorderedMap.end())
			{
				continue;
			}

			sum += it->second * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	void BenchmarkUnorderedMapEraseSetup()
	{
		BenchmarkUnorderedMapEmplaceSetup();
		BenchmarkUnorderedMapEmplace();
	}

	void BenchmarkUnorderedMapErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			g_TestUnorderedMap.erase(e);
		}
		ClobberMemory();
	}
}
