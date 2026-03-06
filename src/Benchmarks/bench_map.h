#ifndef BENCHMARKS_MAP_H
#define BENCHMARKS_MAP_H

namespace Mau
{
	void RegisterMapBenchmarks();

	void BenchmarkMapEmplaceSetup();
	void BenchmarkMapEmplace();

	void BenchmarkMapIterateSetup();
	void BenchmarkMapIterate();

	void BenchmarkMapLookupSetup();
	void BenchmarkMapLookup();

	void BenchmarkMapEraseSetup();
	void BenchmarkMapErase();
}

#endif
