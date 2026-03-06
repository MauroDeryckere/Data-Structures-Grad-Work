#ifndef BENCHMARKS_HIVE_H
#define BENCHMARKS_HIVE_H

namespace Mau
{
	void RegisterHiveBenchmarks();

	void BenchmarkHiveCombinedEmplaceSetup();
	void BenchmarkHiveCombinedEmplace();

	void BenchmarkHiveCombinedIterateSetup();
	void BenchmarkHiveCombinedIterate();

	// Lookup and erase require a side map (unordered_map<Entity, iterator>)
	// since hive is a sequence container with no key-based access
	void BenchmarkHiveCombinedLookupSetup();
	void BenchmarkHiveCombinedLookup();

	void BenchmarkHiveCombinedEraseSetup();
	void BenchmarkHiveCombinedErase();

	void BenchmarkHiveValueOnlyEmplaceSetup();
	void BenchmarkHiveValueOnlyEmplace();

	void BenchmarkHiveValueOnlyIterateSetup();
	void BenchmarkHiveValueOnlyIterate();
}

#endif
