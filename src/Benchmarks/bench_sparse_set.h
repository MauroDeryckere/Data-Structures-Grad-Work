#ifndef BENCHMARKS_SPARSE_SET_H
#define BENCHMARKS_SPARSE_SET_H

namespace Mau
{
	void RegisterSparseSetBenchmarks();

	void BenchmarkSparseSetIterate();
	void BenchmarkSparseSetEmplace();
}

#endif