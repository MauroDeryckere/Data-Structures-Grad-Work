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
	std::string const stdLibInfo{ Mau::GetStdLibInfo() };
	std::cout << "Running benchmarks for: " << compilerInfo << " / " << stdLibInfo << "\n";

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

	// Sanitize compiler+stdlib info for file name (both are needed to keep e.g.
	// Clang/MSVC-STL and Clang/libc++ runs from overwriting each other's CSV)
	std::string safeName{ compilerInfo + "_" + stdLibInfo };
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

	benchmarkReg.WriteCsv(filePath, compilerInfo, stdLibInfo, results);

	std::filesystem::path const mergedFile{ resultsDir / "all_results.csv" };

	benchmarkReg.AppendToMasterResults(mergedFile, compilerInfo, stdLibInfo, results);

	return 0;
}
