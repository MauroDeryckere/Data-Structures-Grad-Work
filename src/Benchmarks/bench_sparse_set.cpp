#include "bench_sparse_set.h"
#include "../benchmark.h"

#include "SparseSet/SparseSet.h"

namespace Mau
{
	sparse_set<ComponentSmall, Entity> g_TestSparseSet;

	void RegisterSparseSetBenchmarks()
	{
		auto& reg{ Mau::BenchmarkRegistry::GetInstance() };

		reg.Register("Sparse Set Emplace", "Emplace", BenchmarkSparseSetEmplace, TEST_ITERATIONS, BenchmarkSparseSetEmplaceSetup);
		reg.Register("Sparse Set Iterate", "Iterate", BenchmarkSparseSetIterate, TEST_ITERATIONS, BenchmarkSparseSetIterateSetup);
		reg.Register("Sparse Set Lookup", "Lookup", BenchmarkSparseSetLookup, TEST_ITERATIONS, BenchmarkSparseSetLookupSetup);
		reg.Register("Sparse Set Erase", "Erase", BenchmarkSparseSetErase, TEST_ITERATIONS, BenchmarkSparseSetEraseSetup);
	}

	void BenchmarkSparseSetEmplaceSetup()
	{
		g_TestSparseSet.clear();
	}

	void BenchmarkSparseSetEmplace()
	{
		for (uint32_t i{ 0 }; i < Mau::TEST_MAP_SIZE; ++i)
		{
			float const value{ Mau::GenerateValue(i) };
			g_TestSparseSet.emplace(i, value);
		}
		ClobberMemory();
	}

	void BenchmarkSparseSetIterateSetup()
	{
		BenchmarkSparseSetEmplaceSetup();
		BenchmarkSparseSetEmplace();
	}

	void BenchmarkSparseSetIterate()
	{
		float sum{ 0.0f };

		for (auto const& item : g_TestSparseSet)
		{
			sum += item * 2.0f;
			DoNotOptimize(sum);
		}
		ClobberMemory();
	}

	void BenchmarkSparseSetLookupSetup()
	{
		BenchmarkSparseSetEmplaceSetup();
		BenchmarkSparseSetEmplace();
	}

	void BenchmarkSparseSetLookup()
	{
		float sum{ 0.0f };

		for (auto const& e : g_LookupKeys)
		{
			auto it{ g_TestSparseSet.find(e) };
			if (it == g_TestSparseSet.end())
			{
				continue;
			}

			sum += *it * 2.0f;
		}

		DoNotOptimize(sum);
		ClobberMemory();
	}

	void BenchmarkSparseSetEraseSetup()
	{
		BenchmarkSparseSetEmplaceSetup();
		BenchmarkSparseSetEmplace();
	}

	void BenchmarkSparseSetErase()
	{
		for (auto const& e : g_LookupKeys)
		{
			g_TestSparseSet.remove(e);
		}
		ClobberMemory();
	}
}
