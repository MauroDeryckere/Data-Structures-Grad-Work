#ifndef BENCHMARKS_UNORDERED_MAP_H
#define BENCHMARKS_UNORDERED_MAP_H

namespace Mau
{
	void RegisterUnorderedMapBenchmarks();

	void BenchmarkUnorderedMapEmplaceSetup();
	void BenchmarkUnorderedMapEmplace();

	void BenchmarkUnorderedMapIterateSetup();
	void BenchmarkUnorderedMapIterate();

	void BenchmarkUnorderedMapLookupSetup();
	void BenchmarkUnorderedMapLookup();

	void BenchmarkUnorderedMapEraseSetup();
	void BenchmarkUnorderedMapErase();
}

#endif
