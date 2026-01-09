#include "bench_sparse_set.h"
#include "../benchmark.h"

#include "SparseSet/SparseSet.h"

namespace Mau
{
	sparse_set<ComponentSmall, Entity> g_TestSparseSet;

	void RegisterSparseSetBenchmarks()
	{
		auto& benchmarkReg{ Mau::BenchmarkRegistry::GetInstance() };

		benchmarkReg.Register("Sparse Set Emplace", "Sparse Set Emplace", BenchmarkSparseSetEmplace, 10);
		benchmarkReg.Register("Sparse Set Iterate", "Sparse Set Iterate", BenchmarkSparseSetIterate, 10);
	}

	void BenchmarkSparseSetIterate()
	{
		static volatile float sum{ 0.0f };

		for (auto& item : g_TestSparseSet)
		{
			sum += item * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkSparseSetEmplace()
	{
		g_TestSparseSet.clear();

		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestSparseSet.emplace(i, value);
		}
	}
}