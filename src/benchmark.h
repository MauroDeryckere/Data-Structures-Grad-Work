#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "singleton.h"

#include <functional>
#include <filesystem>

#include <string>
#include <vector>

#include <optional>
#include <cstdint>

#include "benchmark_utils.h"

namespace Mau
{
	using Entity = uint32_t;

	using ComponentSmall = float; // 4 bytes

	struct ComponentMedium // 64 bytes
	{
		float data[16]{};
	};

	struct ComponentBig // 256 bytes
	{
		float data[64]{};
	};

	uint32_t constexpr TEST_MAP_SIZE{ 1'000'000 };
	uint32_t constexpr TEST_ITERATIONS{ 30 };

	uint32_t constexpr NUM_WARMUP_RUNS{ 5 };

	extern std::vector<Entity> g_LookupKeys;

	void InitLookupKeys(float lookupFraction, float hitRatio);

	using BenchmarkSetupFunc = std::function<void()>;
	using BenchmarkFunc = std::function<void()>;
	using BenchmarkTeardownFunc = std::function<void()>;


	class BenchmarkRegistry final : public MauCor::Singleton<BenchmarkRegistry>
	{
	public:
		struct BenchmarkResult final
		{
			std::string name;
			std::string category;

			size_t iterations;

			double avgMs;
			double totalMs;
			double medianMs;
			double minMs;
			double maxMs;
			double stdDevMs;
		};

		void Register(std::string const& name, std::string const& category, BenchmarkFunc const& func, size_t iterations = TEST_ITERATIONS, BenchmarkSetupFunc const& setupFunc = {}, BenchmarkTeardownFunc const& teardownFunc = {}) noexcept
		{
			m_Benchmarks.emplace_back(name, category, setupFunc, func, teardownFunc, iterations);
		}

		[[nodiscard]] std::vector<BenchmarkResult> RunAll(std::optional<std::vector<std::string>> categoryFilter = std::nullopt) const noexcept;

		static bool WriteCsv(std::filesystem::path const& filePath, std::string const& compilerInfo, std::vector<BenchmarkResult> const& results) noexcept;

		static bool AppendToMasterResults(std::filesystem::path const& mergedFile, std::string const& compilerInfo, std::vector<BenchmarkResult> const& results) noexcept;

	private:
		friend class Singleton<BenchmarkRegistry>;
		BenchmarkRegistry() = default;
		virtual ~BenchmarkRegistry() override = default;

		struct BenchmarkEntry final
		{
			std::string name;
			std::string category;

			BenchmarkSetupFunc setupFunc;
			BenchmarkFunc func;
			BenchmarkTeardownFunc teardownFunc;
			size_t iterations;
		};

		std::vector<BenchmarkEntry> m_Benchmarks;

		static BenchmarkResult RunBenchmark(BenchmarkEntry const& entry) noexcept;
	};

}

#endif
