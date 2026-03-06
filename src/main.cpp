#include <iostream>
#include <filesystem>

#include <string>
#include <vector>

#include "benchmark.h"

#include "Benchmarks/bench_flat_map.h"
#include "Benchmarks/bench_std_flat_map.h"
#include "Benchmarks/bench_map.h"
#include "Benchmarks/bench_hive.h"
#include "Benchmarks/bench_sparse_set.h"
#include "Benchmarks/bench_unordered_map.h"


int main(int argc, char* argv[])
{
	std::string const compilerInfo{ Mau::GetCompilerInfo() };
	std::cout << "Running benchmarks for: " << compilerInfo << "\n";

	// Optional category filter from command line args
	std::optional<std::vector<std::string>> categoryFilter;
	if (argc > 1)
	{
		categoryFilter = std::vector<std::string>{};
		for (int i{ 1 }; i < argc; ++i)
		{
			categoryFilter->emplace_back(argv[i]);
		}

		std::cout << "Filtering categories:";
		for (auto const& cat : *categoryFilter)
		{
			std::cout << " " << cat;
		}
		std::cout << "\n";
	}

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
	Mau::RegisterStdFlatMapBenchmarks();
	Mau::RegisterHiveBenchmarks();
	Mau::RegisterMapBenchmarks();
	Mau::RegisterSparseSetBenchmarks();
	Mau::RegisterUnorderedMapBenchmarks();

	auto& benchmarkReg{ Mau::BenchmarkRegistry::GetInstance() };
	auto const results{ benchmarkReg.RunAll(categoryFilter) };
#pragma endregion

	benchmarkReg.WriteCsv(filePath, compilerInfo, results);

	std::filesystem::path const mergedFile{ resultsDir / "all_results.csv" };

	benchmarkReg.AppendToMasterResults(mergedFile, compilerInfo, results);

	return 0;
}
