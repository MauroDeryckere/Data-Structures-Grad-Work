#ifndef BENCHMARKS_HIVE_H
#define BENCHMARKS_HIVE_H

namespace Mau
{
	void RegisterHiveBenchmarks();
	
	void BenchmarkHiveCombinedIterate();
	void BenchmarkHiveCombinedEmplace();

	void BenchmarkHiveValueOnlyIterate();
	void BenchmarkHiveValueOnlyEmplace();
}

#endif