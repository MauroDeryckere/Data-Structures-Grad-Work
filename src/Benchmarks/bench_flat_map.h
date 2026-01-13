#ifndef BENCHMARKS_FLAT_H
#define BENCHMARKS_FLAT_H

namespace Mau
{
	void RegisterFlatMapBenchmarks();

	void BenchmarkFlatMapEmplaceSetup();
	void BenchmarkFlatMapEmplace();

	void BenchmarkFlatMapIterateSetup();
	void BenchmarkFlatMapIterate();

	void BenchmarkFlatMapLookupSetup();
	void BenchmarkFlatMapLookup();
	
	void BenchmarkFlatMapEraseSetup();
	void BenchmarkFlatMapErase();
}

#endif