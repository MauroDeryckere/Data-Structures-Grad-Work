#ifndef MAU_BENCHMARK_UTILS_H
#define MAU_BENCHMARK_UTILS_H

#if defined(__clang__) || defined(__GNUC__)
template <class T>
inline void DoNotOptimize(const T& value)
{
	asm volatile("" : : "g"(value) : "memory");
}

inline void ClobberMemory()
{
	asm volatile("" : : : "memory");
}

#elif defined(_MSC_VER)
#include <intrin.h>

template <class T>
inline void DoNotOptimize(const T& value)
{
	_ReadWriteBarrier();
	(void)value;
}

inline void ClobberMemory()
{
	_ReadWriteBarrier();
}
#endif

namespace Mau
{
	[[nodiscard]] static std::string GetCompilerInfo() noexcept
	{
		#if defined(__clang__)
			return "Clang " + std::to_string(__clang_major__) + "." +
				std::to_string(__clang_minor__) + "." +
				std::to_string(__clang_patchlevel__);
		#elif defined(_MSC_VER)
			return "MSVC " + std::to_string(_MSC_VER) +
				" (full: " + std::to_string(_MSC_FULL_VER) + ")";
		#elif defined(__GNUC__) && !defined(__clang__)
			return "GCC " + std::to_string(__GNUC__) + "." +
				std::to_string(__GNUC_MINOR__) + "." +
				std::to_string(__GNUC_PATCHLEVEL__);
		#else
			return "Unknown compiler";
		#endif
	}

	[[nodiscard]] static constexpr float GenerateValue(uint32_t i) noexcept
	{
		return static_cast<float>((i * 37) % 1000) / 1000.0f;
	}
}

#endif