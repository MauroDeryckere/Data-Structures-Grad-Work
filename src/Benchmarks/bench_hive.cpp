#include "bench_hive.h"

#include "../Benchmark.h"

#include "plf_hive/plf_hive.h"

namespace Mau
{
	plf::hive<std::pair<int, float>> g_TestHive;

	void RegisterHiveBenchmarks()
	{
		auto& reg = Mau::BenchmarkRegistry::GetInstance();
		reg.Register("Hive Emplace", "Map Emplace", BenchmarkHiveEmplace, 10);
		reg.Register("Hive Iterate", "Map Iterate", BenchmarkHiveIterate, 10);
	}

	void BenchmarkHiveIterate()
	{
		float sum{ 0.0f };
		for (auto& item : g_TestHive)
		{
			sum += item.second * 2.0f;
			DO_NOT_OPTIMIZE(sum);
		}
		CLOBBER_MEMORY();
	}

	void BenchmarkHiveEmplace()
	{
		g_TestHive.clear();
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			g_TestHive.emplace(i, Mau::GenerateValue(i));
		}
	}
}
