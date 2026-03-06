#ifndef BENCHMARKS_SPARSE_SET_H
#define BENCHMARKS_SPARSE_SET_H

namespace Mau
{
	void RegisterSparseSetBenchmarks();

	void BenchmarkSparseSetEmplaceSetup();
	void BenchmarkSparseSetEmplace();

	void BenchmarkSparseSetIterateSetup();
	void BenchmarkSparseSetIterate();

	void BenchmarkSparseSetLookupSetup();
	void BenchmarkSparseSetLookup();

	void BenchmarkSparseSetEraseSetup();
	void BenchmarkSparseSetErase();
}

#endif
