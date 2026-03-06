#include "bench_std_flat_map.h"
#include "../benchmark.h"

#if __has_include(<flat_map>)
	#include <flat_map>
	#ifdef __cpp_lib_flat_map
		#define HAS_STD_FLAT_MAP 1
	#endif
#endif

#ifdef HAS_STD_FLAT_MAP

namespace Mau
{
	std::flat_map<Entity, ComponentSmall> g_TestStdFlatMap;

	static void BenchmarkStdFlatMapEmplaceSetup()
	{
		g_TestStdFlatMap.clear();
	}

	static void BenchmarkStdFlatMapEmplace()
	{
		for (uint32_t i{ 0 }; i < TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestStdFlatMap.emplace(i, value);
		}
		ClobberMemory();
	}

	static void BenchmarkStdFlatMapIterateSetup()
	{
		BenchmarkStdFlatMapEmplaceSetup();
		BenchmarkStdFlatMapEmplace();
	}

	static void BenchmarkStdFlatMapIterate()
	{
		float sum{ 0.0f };

		for (auto const& [key, val] : g_TestStdFlatMap)
		{
			sum += val * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	static void BenchmarkStdFlatMapLookupSetup()
	{
		BenchmarkStdFlatMapEmplaceSetup();
		BenchmarkStdFlatMapEmplace();
	}

	static void BenchmarkStdFlatMapLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_TestStdFlatMap.find(e) };
			if (it == g_TestStdFlatMap.end())
			{
				continue;
			}

			sum += it->second * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	static void BenchmarkStdFlatMapEraseSetup()
	{
		BenchmarkStdFlatMapEmplaceSetup();
		BenchmarkStdFlatMapEmplace();
	}

	static void BenchmarkStdFlatMapErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			g_TestStdFlatMap.erase(e);
		}
		ClobberMemory();
	}

	void RegisterStdFlatMapBenchmarks()
	{
		auto& reg{ BenchmarkRegistry::GetInstance() };

		reg.Register("Std Flat Map Emplace", "Emplace", BenchmarkStdFlatMapEmplace, TEST_ITERATIONS, BenchmarkStdFlatMapEmplaceSetup);
		reg.Register("Std Flat Map Iterate", "Iterate", BenchmarkStdFlatMapIterate, TEST_ITERATIONS, BenchmarkStdFlatMapIterateSetup);
		reg.Register("Std Flat Map Lookup", "Lookup", BenchmarkStdFlatMapLookup, TEST_ITERATIONS, BenchmarkStdFlatMapLookupSetup);
		reg.Register("Std Flat Map Erase", "Erase", BenchmarkStdFlatMapErase, TEST_ITERATIONS, BenchmarkStdFlatMapEraseSetup);
	}
}

#else

namespace Mau
{
	void RegisterStdFlatMapBenchmarks()
	{
		// std::flat_map not available on this compiler
	}
}

#endif
