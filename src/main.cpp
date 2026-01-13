#include <iostream>
#include <filesystem>

#include <map>
#include <unordered_map>

#include <string>
#include <vector>

#include "benchmark.h"

#include "Benchmarks/bench_flat_map.h"
#include "Benchmarks/bench_map.h"
#include "Benchmarks/bench_hive.h"
#include "Benchmarks/bench_sparse_set.h"
#include "Benchmarks/bench_unordered_map.h"


int main()
{
	std::string const compilerInfo{ Mau::GetCompilerInfo() };
	std::cout << "Running benchmarks for: " << compilerInfo << "\n";

	// Sanitize compiler info for file name
	std::string safeName{ compilerInfo };
	for (char& c : safeName) 
	{
		if (c == ' ' || c == '(' || c == ')' || c == ':') c = '_';
	}

	std::filesystem::path const resultsDir{ std::filesystem::path(PROJECT_RESULTS_DIR) };
	std::filesystem::create_directories(resultsDir);
	std::filesystem::path const filePath{ resultsDir / ("bench_results_" + safeName + ".csv") };

#pragma region benchmarking
	Mau::InitLookupKeys(0.5f, 0.8f);

	Mau::RegisterFlatMapBenchmarks();
	Mau::RegisterHiveBenchmarks();
	Mau::RegisterMapBenchmarks();
	Mau::RegisterSparseSetBenchmarks();
	Mau::RegisterUnorderedMapBenchmarks();

	auto& benchmarkReg{ Mau::BenchmarkRegistry::GetInstance() };
	auto const results{ benchmarkReg.RunAll() };
#pragma endregion

	benchmarkReg.WriteCsv(filePath, compilerInfo, results);

	std::filesystem::path const mergedFile{ resultsDir / "all_results.csv" };

	benchmarkReg.AppendToMasterResults(mergedFile, compilerInfo, results);

	return 0;
}