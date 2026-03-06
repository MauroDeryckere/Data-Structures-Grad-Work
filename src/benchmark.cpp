#include "benchmark.h"

#include <algorithm>
#include <numeric>
#include <iostream>
#include <fstream>
#include <chrono>
#include <random>
#include <cmath>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace Mau
{
	std::vector<Entity> g_LookupKeys;

	void InitLookupKeys(float lookupFraction, float hitRatio)
	{
		g_LookupKeys.clear();

		size_t const lookupCount{ static_cast<size_t>(TEST_MAP_SIZE * lookupFraction) };
		g_LookupKeys.resize(lookupCount);

		std::mt19937 rng(12345);

		std::bernoulli_distribution hitDist(hitRatio);

		std::uniform_int_distribution<Entity> hitKeyDist(0, TEST_MAP_SIZE - 1);
		std::uniform_int_distribution<Entity> missKeyDist(TEST_MAP_SIZE, TEST_MAP_SIZE * 2);

		for (auto& key : g_LookupKeys)
		{
			key = hitDist(rng) ? hitKeyDist(rng) : missKeyDist(rng);
		}

		std::shuffle(g_LookupKeys.begin(), g_LookupKeys.end(), rng);
	}

	std::vector<BenchmarkRegistry::BenchmarkResult> BenchmarkRegistry::RunAll(std::optional<std::vector<std::string>> categoryFilter) const noexcept
	{
		std::vector<BenchmarkResult> results;
		results.reserve(m_Benchmarks.size());

		for (auto const& b : m_Benchmarks)
		{
			if (categoryFilter)
			{
				auto const& filters{ (*categoryFilter) };
				if (!filters.empty() &&
					std::find(filters.begin(), filters.end(), b.category) == filters.end())
				{
					continue;
				}
			}

			results.emplace_back(RunBenchmark(b));
		}

		return results;
	}

	BenchmarkRegistry::BenchmarkResult BenchmarkRegistry::RunBenchmark(BenchmarkEntry const& entry) noexcept
	{
		using namespace std::chrono;

		assert(entry.iterations > 2);

		std::vector<double> times;
		times.reserve(entry.iterations);

		for (size_t i{ 0 }; i < NUM_WARMUP_RUNS; ++i)
		{
			if (entry.setupFunc)
			{
				entry.setupFunc();
			}

			if (entry.func)
			{
				entry.func();
			}

			if (entry.teardownFunc)
			{
				entry.teardownFunc();
			}
		}

		for (size_t i{ 0 }; i < entry.iterations; ++i)
		{
			if (entry.setupFunc)
			{
				entry.setupFunc();
			}

			auto const start{ high_resolution_clock::now() };

			if (entry.func)
			{
				entry.func();
			}

			auto const end{ high_resolution_clock::now() };

			auto const dur{ duration<double, std::milli>(end - start).count() };
			times.emplace_back(dur);

			if (entry.teardownFunc)
			{
				entry.teardownFunc();
			}
		}

		std::sort(times.begin(), times.end());

		auto const n{ times.size() };
		size_t const trim{ std::max<size_t>(1, n / 10) };

		auto const begin{ times.begin() + trim };
		auto const end{ times.end() - trim };
		size_t const used{ static_cast<size_t>(std::distance(begin, end)) };

		double const total{ std::accumulate(begin, end, 0.0) };
		double const avg{ total / used };
		double const median{ begin[used / 2] };
		double const min{ *begin };
		double const max{ *(end - 1) };

		double sumSqDiff{ 0.0 };
		for (auto it = begin; it != end; ++it)
		{
			double const diff{ *it - avg };
			sumSqDiff += diff * diff;
		}
		double const stdDev{ std::sqrt(sumSqDiff / used) };

		return { entry.name, entry.category, used, avg, total, median, min, max, stdDev };
	}

	bool BenchmarkRegistry::WriteCsv(std::filesystem::path const& filePath, std::string const& compilerInfo, std::vector<BenchmarkResult> const& results) noexcept
	{
		std::ofstream out(filePath);
		if (!out.is_open())
		{
			std::cerr << "Error: could not write to " << filePath << "\n";
			return false;
		}

		out.imbue(std::locale::classic());
		out << std::fixed << std::setprecision(6);
		out << "Compiler,Benchmark,Category,Iterations,Average(Ms),Total(Ms),Median(Ms),Min(Ms),Max(Ms),StdDev(Ms)\n";

		for (auto const& r : results)
		{
			out << compilerInfo << ','
				<< r.name << ','
				<< r.category << ','
				<< r.iterations << ','
				<< r.avgMs << ','
				<< r.totalMs << ','
				<< r.medianMs << ','
				<< r.minMs << ','
				<< r.maxMs << ','
				<< r.stdDevMs << '\n';
		}

		std::cout << "\nResults written to: " << filePath << "\n";
		return true;
	}

	bool BenchmarkRegistry::AppendToMasterResults(std::filesystem::path const& mergedFile, std::string const& compilerInfo, std::vector<BenchmarkResult> const& results) noexcept
	{
		std::vector<std::string> oldLines;
		if (std::filesystem::exists(mergedFile))
		{
			std::ifstream in(mergedFile);
			std::string line;
			int skip{ 0 };
			while (std::getline(in, line))
			{
				if (skip != 2)
				{
					++skip;
					continue;
				}

				oldLines.emplace_back(line);
			}
		}

		for (auto const& r : results)
		{
			std::ostringstream oss;
			oss << std::fixed << std::setprecision(6);
			oss << compilerInfo << ','
				<< r.name << ','
				<< r.category << ','
				<< r.iterations << ','
				<< r.avgMs << ','
				<< r.totalMs << ','
				<< r.medianMs << ','
				<< r.minMs << ','
				<< r.maxMs << ','
				<< r.stdDevMs;

			oldLines.emplace_back(oss.str());
		}

		auto getField
		{
			[](std::string const& line, size_t index) -> std::string
			{
				size_t start{ 0};
				for (size_t i{ 0 }; i < index; ++i)
				{
					start = line.find(',', start) + 1;
				}
				size_t const end{ line.find(',', start) };

				return line.substr(start, end - start);
			}
		};

		std::sort(oldLines.begin(), oldLines.end(),
			[getField](std::string const& a, std::string const& b)
			{
				std::string const aCategory{ getField(a, 2) };
				std::string const bCategory{ getField(b, 2) };

				if (aCategory == bCategory)
				{
					std::string const aName{ getField(a, 1) };
					std::string const bName{ getField(b, 1) };
					return aName < bName;
				}

				return aCategory < bCategory;
			});


		std::ofstream merged(mergedFile, std::ios::trunc);
		if (!merged.is_open())
		{
			std::cerr << "Error: could not write to " << mergedFile << "\n";
			return false;
		}

		merged.imbue(std::locale::classic());
		merged << std::fixed << std::setprecision(6);

		auto const now = std::chrono::system_clock::now();
		std::time_t const now_c = std::chrono::system_clock::to_time_t(now);
		std::tm tm{};
	#ifdef _WIN32
		localtime_s(&tm, &now_c);
	#else
		localtime_r(&now_c, &tm);
	#endif
		char timeBuf[64];
		std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tm);

		merged << "Date:," << timeBuf << "\n";

		merged << "Compiler,Benchmark,Category,Iterations,Average(Ms),Total(Ms),Median(Ms),Min(Ms),Max(Ms),StdDev(Ms)\n";
		for (auto const& line : oldLines)
		{
			merged << line << "\n";
		}

		std::cout << "Appended results to: " << mergedFile << "\n";
		return true;
	}
}
