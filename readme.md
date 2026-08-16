# Data Structures Grad Work — ECS Container Benchmarks

Benchmark harness comparing `std::flat_map`, `std::hive` (via `plf::hive`), `std::map`,
`std::unordered_map`, the SG14 reference `flat_map`, and a hand-rolled `sparse_set` as
candidate ECS component storage, across MSVC, GCC/libstdc++, Clang/MSVC-STL, and
Clang/libc++.

## Prerequisites

- Windows 10 (1803+) or Windows 11 — `tar.exe` (bundled) is used for archive extraction.
- Visual Studio 2022 (17.x) or Visual Studio 2026 (18.x) with the **Desktop development
  with C++** workload. `tools/configs/msvc-2022-14.38.33135.vsconfig` can be imported into
  the VS Installer to get the exact component set.
- [winget](https://learn.microsoft.com/en-us/windows/package-manager/winget/) available on
  `PATH` (ships with modern Windows) — used to install MSYS2, CMake, and Ninja.
- Internet access for the one-time toolchain downloads (~1–2 GB total: WinLibs GCC builds,
  an LLVM Windows release, MSYS2 packages).

## How to use

```powershell
# 1. One-time setup: installs/detects every toolchain the suite builds with.
tools/install.ps1

# 2. Build + run the benchmark suite with every available toolchain.
tools/run_tests.ps1

# 3. Wipe accumulated results (e.g. before a clean re-run for the paper).
tools/clear_results.ps1
```

`tools/run_tests.ps1` supports two optional switches for faster iteration:

```powershell
# Only rebuild/run specific toolchains
tools/run_tests.ps1 -Only gcc,clang-libcxx

# Only run specific benchmark categories (forwarded to Project.exe)
tools/run_tests.ps1 -Categories Iterate,Lookup

# Both together
tools/run_tests.ps1 -Only msvc -Categories Emplace
```

Valid `-Only` values: `msvc`, `gcc`, `clang`, `clang-libcxx` (default: all four).

### Troubleshooting

**`"running scripts is disabled on this system"`** — PowerShell's default execution policy
(`Restricted`) blocks all `.ps1` files from running directly. Either invoke scripts via
`powershell -ExecutionPolicy Bypass -File tools\run_tests.ps1` every time, or set it once for
your account (a security-relevant setting, so apply it yourself rather than scripting it):
`Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`. `RemoteSigned` is the standard safe
choice for local development — it only restricts scripts downloaded from the internet,
locally-created ones (like these) run freely.

**`"Program 'Project.exe' failed to run: An Application Control policy has blocked this
file"`** — Windows 11's **Smart App Control** blocking a freshly-built, unsigned binary that
hasn't earned reputation yet. Confirm via `Get-MpComputerStatus | Select
SmartAppControlState`, and via the `Microsoft-Windows-CodeIntegrity/Operational` event log
(`Get-WinEvent -FilterHashtable @{LogName='Microsoft-Windows-CodeIntegrity/Operational';
Id=3033,3077}`), which reports "did not meet the Enterprise signing level requirements."
This is evaluated per binary hash and appears **non-deterministic across which toolchain leg
it hits on a given run** — it blocked the Clang/MSVC-STL leg once in testing and let the
identical binary through unmodified on a plain retry. If you hit this:
- Just retry `tools/run_tests.ps1 -Only <leg>` — often succeeds on a second attempt with no
  changes.
- Smart App Control cannot be toggled off once enabled without a full "Reset this PC"
  (Microsoft's design choice) — not worth it just for this.
- Worth noting as a genuine reproducibility caveat for the paper: on a machine with Smart App
  Control on, a run can spuriously fail to produce a result for some toolchain leg, for
  reasons entirely unrelated to anything under test.

### What `install.ps1` sets up

| Toolchain | Source | Pin | Standard library |
|---|---|---|---|
| MSVC | Visual Studio (detected, not installed) | whatever VS is present | MSVC STL |
| GCC | WinLibs portable build | `tools/gcc_versions.txt` (13.2.0, 14.2.0, 15.2.0) | libstdc++ |
| Clang | Official LLVM Windows release | `tools/clang_versions.txt` (21.1.2) | MSVC STL |
| Clang | MSYS2 `CLANG64` environment (`pacman`) | rolling, MSYS2-managed | **libc++** |

Why two of the four legs both link MSVC STL: the official LLVM Windows release targets
`x86_64-pc-windows-msvc` and doesn't ship a libc++ built for that ABI, so `clang++` from
that release falls back to whatever Visual Studio provides (see Sources). That's not
redundant for RQ3 — comparing MSVC-the-compiler+MSVC-STL against Clang-the-compiler+MSVC-STL
isolates the *compiler's* effect while holding the standard library implementation constant,
which the MSYS2 `CLANG64` leg then contrasts against genuine libc++ on the same compiler.

GCC and Clang versions are pinned by URL in `tools/gcc_versions.txt` /
`tools/clang_versions.txt` for reproducibility; re-running `install.ps1` on any machine
fetches the exact same builds. MSVC and MSYS2's `CLANG64` package are not pinned to a
specific build (Visual Studio has no portable/zip distribution to pin, and MSYS2 packages
roll forward) — the exact version actually used on a given run is always recorded in the
output (see below), so results stay attributable even though the *installed* version isn't
locked.

## Build flags & linking

### Optimization

| Toolchain | Flags |
|---|---|
| MSVC | `/O2 /DNDEBUG /GL- /GS- /std:c++latest`, link: `/LTCG:OFF` |
| GCC / Clang (all 3 non-MSVC legs) | `-O3 -march=native -DNDEBUG -fno-lto` |

Every leg also sets `INTERPROCEDURAL_OPTIMIZATION OFF` at the CMake level.

**Why LTO/whole-program optimization is disabled everywhere:** LTO aggressiveness and
availability differ substantially across GCC's LTO plugin, Clang's ThinLTO, and MSVC's `/GL`
— enabling it non-uniformly would confound "which compiler generates better per-TU code"
(the thing under test) with "whose whole-program optimizer happens to be more mature" (not
the thing under test). Disabled everywhere as a controlled variable
(`-fno-lto` / `/GL- /LTCG:OFF` / `INTERPROCEDURAL_OPTIMIZATION OFF`, belt-and-suspenders on
the non-MSVC legs).

**Why `-march=native`:** targets the exact host CPU's instruction set rather than a generic
x86-64 baseline, for realistic peak throughput. **Reproducibility caveat for the paper:**
this ties *absolute* timings to the specific CPU they were measured on — cross-machine
absolute-time comparisons are not valid, only relative (container A vs container B on the
*same* run) comparisons are. Known gap: the CSV output doesn't currently record CPU
model/frequency, which the assignment's own "Subjects Tested" section calls for — to be
added in the benchmark-code phase, not this tooling pass.

**Why `/arch:AVX2` was added for MSVC (GCC/Clang already had the equivalent via
`-march=native`):** MSVC has no "native" auto-detect flag — `/arch:` requires picking a fixed
level — so it was initially left unset, meaning MSVC alone was generating narrower code than
the other three legs on the same CPU. This is a verified fact, not a benchmark-derived one:

- Queried this machine directly (`gcc -march=native -E -v`, see Sources): the host CPU (an
  Intel Arrow Lake-S) resolves to `-march=arrowlake-s` with `-mavx -mavx2 -mfma -mbmi -mbmi2
  -maes -mvaes` among its enabled features (no AVX-512 — Intel doesn't ship it on this
  generation of consumer chips) — so GCC/Clang's `-march=native` was already using this
  CPU's full AVX2+FMA+BMI2+AES/VAES capability on their three legs.
- Per Microsoft's own `/arch` (x64) documentation (see Sources): *"The default instruction
  set is SSE2 if no `/arch` option is specified... The default mode uses SSE2 instructions
  for scalar floating-point and vector calculations. These instructions allow calculation
  with 128-bit vectors..."* — confirming MSVC's un-opted-in baseline is 128-bit SSE2, not
  something that scales with the actual host CPU. The same page confirms `/arch:AVX2`
  *"extends most integer operations to 256-bit vectors and enables use of Fused Multiply-Add
  (FMA) instructions"* and that "`/arch:AVX2` may also enable... certain BMI instructions" —
  i.e. `/arch:AVX2` is the closest available MSVC level to what GCC/Clang's `-march=native`
  already picked for this CPU (AES/VAES aren't covered by any single `/arch:` level and would
  need explicit intrinics-level opt-in, a smaller residual gap left undocumented-as-fixed).

This is fixed on the strength of that verified, timing-independent fact — not on a measured
speedup. Empirically testing it hit the same between-run noise problem documented below (see
"Large uncontrolled between-run variance on this machine"), so no reliable before/after
percentage is claimed here; the fix is justified because leaving MSVC un-opted-in while
GCC/Clang were explicitly opted in via `-march=native` was an inconsistent choice on this
project's part, not something to leave unaddressed just because its magnitude isn't
measurable at smoke-test scale.

**Why `-ffast-math` was tried and then removed (GCC/Clang originally had it, MSVC never
did):** `-ffast-math` is a bundle flag (`-fassociative-math`, `-freciprocal-math`,
`-ffinite-math-only`, `-fno-signed-zeros`, `-fno-trapping-math`, `-fno-math-errno`) that
trades IEEE-754 conformance for optimization freedom — critically,
`-fassociative-math` lets the compiler reorder/reassociate floating-point additions, which
is a genuine *semantic* change (floating-point addition isn't associative; reordering it
changes the computed result, not just the time to compute it), unlike `-O2`/`-O3` which
preserve exact observable behavior. This project's benchmarks are testing **container
mechanics — insertion, lookup, iteration, erasure cost — not floating-point arithmetic
throughput**, so giving GCC/Clang extra numerical license that MSVC doesn't get would
confound "which compiler generates better code for the container operations under test"
(the actual RQ3 question) with "which compiler's fast-math reassociation happens to be more
aggressive" (not a research question here at all). Removed for strict parity across all four
legs rather than added to MSVC, since stricter IEEE semantics is the safer default for a
structure-comparison benchmark. Practically low-cost: every `Iterate` benchmark calls
`DoNotOptimize(sum)` *inside* the loop on every iteration (a compiler memory-barrier
intrinsic), which already blocks the cross-iteration reassociation `-fassociative-math`
would otherwise enable — so removing the flag is expected to leave those numbers largely
unaffected; only `Lookup`'s post-loop summation had any real reassociation license to lose,
and that summation is incidental to the dominant `find()` cost being measured there. This
reasoning is from code inspection, not an A/B measurement — worth a quick before/after
comparison once real (non-smoke-test) data is collected, rather than taken as confirmed.

**Why `/GS-` (MSVC only; GCC/Clang never had the equivalent):** MSVC's `/GS` (buffer security
check — stack cookies inserted around functions with local buffers, checked on return) is
**on by default even in Release builds**; Microsoft's own documentation and independent
benchmarking cite overhead "in some tests... more than 10%," depending on how much local
buffer usage the code has. GCC/Clang's equivalent, `-fstack-protector`, is **not enabled by
default on mingw-w64 targets** — confirmed via MSYS2/mingw-w64 issue tracker discussion, this
isn't a distro-patched default the way it sometimes is on Linux. So MSVC was paying a
security-check tax the other three legs never had, for a mitigation unrelated to what's under
test. Also checked and ruled out as a separate concern: **Control Flow Guard** (`/guard:cf`,
which specifically taxes indirect calls — relevant since every timed benchmark call goes
through `std::function::operator()`) is confirmed **off by default** for MSVC, so no
asymmetry there.

Empirically tested across three back-to-back runs (10k-element smoke-test scale) rather than
just applied on faith:
- Run 1 (before → after `/GS-`) showed large, low-StdDev improvements on several benchmarks
  (`Sparse Set Lookup` -45.6%, `Flat Map Lookup` -42.6%, `Map Erase` -45.7%), consistent with
  `/GS`'s documented cost — but also one tight, consistently-higher reading (`Map Emplace`
  +14.7%) that had no plausible mechanism (`/GS-` cannot make code slower, only measurement
  noise can).
- Run 2 (same binary, no code change) came back **faster across nearly every benchmark, not
  just the ones `/GS-` should affect** — e.g. `Flat Map Emplace` 0.162 → 0.133 → 0.069ms,
  `Sparse Set Iterate` 0.0075 → 0.0075 → 0.0043ms — and `Map Emplace`'s apparent regression
  from run 1 **did not reproduce** (0.891 → 1.021 → 0.625ms, now faster than the original
  pre-fix number).

**Conclusion:** kept `/GS-` on the strength of Microsoft's documented mechanism plus "no
run produced evidence against it," but the precise magnitude is **not reliably quantifiable
from this dataset** — the across-the-board run-to-run shift with zero code change between
runs 2 and 3 shows smoke-test-scale (10k elements, 3 kept samples) between-run system noise
is large enough (up to ~2x) to swamp the flag's own effect. This is itself a useful,
concrete data point (not just a caveat) for the paper: **don't trust single-run smoke-test
comparisons for precise percentages** — only the full 1,000,000-element/30-iteration runs,
averaged over far more samples, should be used for quantitative claims. Separately,
`std::map`'s `Emplace`/`Erase` benchmarks specifically showed high *within-run* variance too
(StdDev up to ~19-20% of the mean on 3 samples) across all three runs, independent of
`/GS-` — plausibly due to sensitivity to per-process heap-fragmentation/ASLR state from many
small individual node allocations — worth watching at full scale rather than assumed to
average out.

### Static linking

Every leg links statically where the toolchain allows it: MSVC via
`CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded` (`/MT`), GCC via `-static -static-libgcc
-static-libstdc++` (CMake's `MINGW` branch), and Clang/MSVC-STL via the same
`CMAKE_MSVC_RUNTIME_LIBRARY` setting so it actually matches the native MSVC leg instead of
silently defaulting to a dynamic CRT.

**Why:** a fully static executable (a) needs no VC++ redistributable / DLL version present on
the target machine — directly serves the "works anywhere" goal — and (b) avoids import-thunk
indirection on every CRT call (`malloc`/`free`, locale, etc.), which would otherwise leak into
allocation-heavy benchmarks (`Map`/`UnorderedMap` Emplace/Erase) as noise unrelated to the
container itself.

**Verified via `dumpbin /DEPENDENTS`, not assumed:** MSVC-native links only `KERNEL32.dll`.
Before this pass's fix, Clang/MSVC-STL pulled in `MSVCP140.dll`, `VCRUNTIME140.dll`, and the
full `api-ms-win-crt-*.dll` UCRT forwarder set — a real confound against the MSVC-native leg,
since those two exist specifically to isolate the compiler's effect while holding the
standard library implementation constant. Fixed by the `CMAKE_MSVC_RUNTIME_LIBRARY` change
above.

**Known remaining limitation:** the MSYS2 `CLANG64` (libc++) leg cannot achieve full static
CRT linking. `dumpbin` confirms `libc++`/`libc++abi`/`libunwind` *are* statically linked (no
such DLL appears in the dependency list), but the underlying UCRT forwarders remain dynamic.
This is an inherent MSYS2/mingw-w64 constraint (UCRT-variant MinGW builds can't be fully
static-linked, the same root cause behind the GCC 14.2.0 repin below) — MSYS2's `CLANG64`
environment is UCRT-only, there's no msvcrt-variant Clang toolchain available there. It's the
one leg out of six where CRT linkage genuinely can't be brought in line with the others.

### Alternatives researched and rejected

| Rejected | Why it doesn't work | Replaced with |
|---|---|---|
| 7-Zip for archive extraction | Unstated external dependency; not present on a clean machine | Windows-bundled `tar.exe` (bsdtar/libarchive), handles `.zip`/`.tar.xz` natively |
| MSYS2 rolling (`pacman -Syu`) GCC as a benchmarked compiler | Not version-pinned — a fresh install months later could silently get a different GCC minor version, breaking reproducibility | Version-pinned WinLibs GCC builds only (`gcc_versions.txt`); MSYS2 kept solely to host `CLANG64` |
| Hardcoded `"Visual Studio 17 2022"` CMake generator for the MSVC leg | Fails outright on non-2022 VS installs — confirmed on this machine, which runs VS 2026 (18.0) | Ninja + a `vcvarsall.bat`-loaded environment (VS-version-agnostic) |
| Official LLVM Windows release as a libc++ source | Targets the `x86_64-pc-windows-msvc` triple and links MSVC STL — no libc++ built for the MSVC ABI ships in that release | MSYS2 `CLANG64` environment (`mingw-w64-clang-x86_64-libc++`), genuine libc++ on Windows |
| GCC 14.2.0 original pin (mcf-threaded, UCRT) | UCRT-variant MinGW builds don't fully static-link even with `-static` — confirmed via `dumpbin` (pulls in the full `api-ms-win-crt-*.dll` set + `ntdll.dll`) | Repinned to a posix-threaded, msvcrt-runtime WinLibs build, matching 13.2.0/15.2.0's runtime family |
| Dynamic CRT (CMake default) for Clang/MSVC-STL | Didn't match the native MSVC leg's static `/MT` CRT — confirmed via `dumpbin` | Explicit `CMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`, matching the native MSVC leg |
| `-ffast-math` on GCC/Clang only | Gave GCC/Clang floating-point reassociation license MSVC never had, confounding container-mechanics comparisons with compiler-specific FP optimization; the suite tests the data structures, not the arithmetic | Removed everywhere for strict IEEE-754 parity across all four legs |
| `/GS` left at its MSVC default | On by default even in Release; GCC/Clang's `-fstack-protector` equivalent is off by default on mingw-w64 targets — MSVC alone was paying a security-check tax unrelated to what's under test | `/GS-` added to the MSVC leg for parity; see "Why `/GS-`" above for the (noisy, smoke-test-scale) empirical check |
| Control Flow Guard (`/guard:cf`) — investigated, no fix needed | Confirmed off by default for MSVC, so no asymmetry actually exists here despite the harness's heavy `std::function` indirect-call usage | Nothing changed — documented as checked-and-cleared rather than left silently unexamined |
| No `/arch:` flag for MSVC | MSVC's un-opted-in default is 128-bit SSE2 (Microsoft-documented), while GCC/Clang's `-march=native` already used this CPU's full AVX2/FMA/BMI2/AES/VAES capability — MSVC alone was targeting hardware from over a decade before this CPU | `/arch:AVX2` added, matching the closest available MSVC level to what the other three legs already use |
| GCC/Clang silently shipping default hardening (`--enable-default-ssp`, `_FORTIFY_SOURCE`) — investigated, no fix needed | A WinLibs release note raised the possibility that some past builds were configured with `--enable-default-ssp` (which would bake in stack-protector by default, the same class of hidden-default issue found in MSYS2's libc++ hardening mode); this would have undermined the whole basis for the `/GS-` fix if it applied to our pinned builds | Checked directly (`strings` on all 5 compiled non-MSVC `Project.exe` binaries for `__stack_chk_fail`/`_chk@`/fortify symbols): all clean. Confirmed our specific pinned GCC 13.2.0/14.2.0/15.2.0 and both Clang legs have no default hardening baked in — the `/GS-` reasoning holds |

## Results

Each toolchain run writes `results/bench_results_<Compiler>_<StdLib>.csv`, and every run
also merges into `results/all_results.csv` (sorted by category, then benchmark name), so
cross-toolchain comparison is a single file. `results/` is git-ignored — export or attach
whatever subset you need for the paper.

The merge is a **replace-by-key**, not a blind append: `AppendToMasterResults`
(`src/benchmark.cpp`) keys each row by `(Compiler, StdLib, Benchmark, Category)`, and any
existing row sharing that key with the current run gets replaced rather than duplicated.
This means re-running a subset of toolchains (`-Only gcc`, say) updates only that
toolchain's rows in `all_results.csv` — everything else stays as it was — and you don't need
to run `clear_results.ps1` between runs just to avoid stale/duplicate rows piling up under
the same label. `clear_results.ps1` is still there for a genuine clean-slate reset (e.g.
before a final data-collection pass for the paper).

CSV columns: `Compiler, StdLib, Benchmark, Category, Iterations, Average(Ms), Total(Ms),
Median(Ms), Min(Ms), Max(Ms), StdDev(Ms)`. `Iterations` is the number of *kept* runs after
discarding the top/bottom ~10% (`BenchmarkRegistry::RunBenchmark` in
`src/benchmark.cpp`) — warm-up runs (`NUM_WARMUP_RUNS`, `src/benchmark.h`) are not
included at all. `Compiler`/`StdLib` are self-reported per-binary via
`GetCompilerInfo()`/`GetStdLibInfo()` (`src/benchmark_utils.h`), so results stay correctly
attributed even for the unpinned MSVC/MSYS2-Clang legs.

## Language/library availability check (Aug 2026)

Before continuing the benchmark implementation, we checked whether either substitute
container the project relies on now has a native standard-library equivalent worth
switching to:

- **`std::hive` (P0447R28)** — voted into C++26, but as of Aug 2026 **not implemented in
  libstdc++, libc++, or MSVC STL** (confirmed against cppreference's C++26 compiler support
  table, which shows blank support columns for the `<hive>` header). `plf::hive` — the
  reference implementation the proposal is formalized from — remains the only available
  option and is what this project benchmarks against `sparse_set`.
- **`std::flat_map` (P0429R9)** — already shipping natively: libstdc++ 15+, libc++ 20+, MSVC
  STL 19.51+ (behind `/std:c++latest` until it's the non-preview default). `gcc_versions.txt`
  now pins GCC 15.2.0 specifically to get native-`std::flat_map` coverage, and
  `CMakeLists.txt` sets `/std:c++latest` for MSVC for the same reason. `src/Benchmarks/
  bench_std_flat_map.cpp` already feature-gates on `__cpp_lib_flat_map`, so it activates
  automatically wherever the underlying library supports it and no-ops elsewhere (e.g. GCC
  13.2.0/14.2.0) — no code changes were needed there once the toolchains above updated.

## Notable findings (from pipeline verification, smoke-test scale)

These surfaced while verifying the tooling end-to-end at a temporarily shrunk dataset size
(`TEST_MAP_SIZE`/`TEST_ITERATIONS` set to 10,000/5 for fast iteration, not the real
1,000,000/30 experimental config) — worth reconfirming at full scale once real data
collection begins, but documented now since the mechanism behind them doesn't depend on
dataset size.

### Large uncontrolled between-run variance on this machine

Observed twice, independently, both times as a uniform swing across *every* benchmark
simultaneously with **zero code changes** between runs — not a per-container or per-flag
effect:
- Between two back-to-back MSVC runs while testing `/GS-` (identical binary): run 2 came back
  roughly 2x faster than run 1 across nearly every benchmark (e.g. `Flat Map Emplace` 0.133 →
  0.069ms, `Sparse Set Iterate` 0.0075 → 0.0043ms).
- Between the `/GS-`-only run and the `/GS-` + `/arch:AVX2` run: the reverse — every
  benchmark came back 60-180% *slower*, again uniformly across completely different code
  paths (`std::map`, `unordered_map`, `sparse_set`, `hive`).

A single flag or container implementation cannot plausibly cause a uniform swing across every
benchmark type simultaneously — tree inserts, hash lookups, and linear iteration don't share
a code path. This points to something external to the code under test: machine-wide state
varying between runs (background system load, thermal/frequency scaling, or possibly
Windows Defender/Smart App Control doing reputation-scanning work on freshly-built binaries —
plausible given this machine already demonstrated Smart App Control actively intercepting a
freshly-built `Project.exe`, see Troubleshooting above).

**Consequence for the paper's methodology:** at smoke-test scale (10,000 elements, 3 kept
samples after trimming) this machine's between-run noise is large enough (~2-3x observed) to
swamp real, individually-small flag effects. Single-run comparisons at this scale are **not**
reliable for quantitative claims — only the full 1,000,000-element/30-iteration runs,
averaged over far more samples, should be used for percentage/magnitude claims in the paper.
This isn't a reason to distrust the flag fixes made this pass (`-ffast-math` removal, libc++
hardening mode, `/GS-`, `/arch:AVX2`) — each is justified independently by documented
compiler/library behavior, not by a smoke-test-measured percentage — but it is a reason to
treat every specific percentage quoted in this document as illustrative, not final.

### libc++'s `std::vector::erase()` has no memmove fast path for trivially-copyable elements

`Flat Map Erase`/`Std Flat Map Erase` (both `std::vector`-backed) ran ~10-20x slower on the
Clang/libc++ (MSYS2 CLANG64) leg than on every other toolchain, while every *other* container
on that same binary was fine — often the fastest of all six. Investigated rather than
dismissed as noise (all 3 kept samples were consistently high — StdDev ~6ms on a ~30ms mean —
not one outlier skewing an average):

1. Ruled out libc++'s hardening mode as the primary cause: forcing
   `_LIBCPP_HARDENING_MODE_NONE` reduced the gap by only ~30% (30.5ms → 22.1ms for
   `Flat Map Erase`), not the ~15-20x needed to close it. Confirmed via libc++'s own
   documentation that its *upstream* default hardening mode is actually `none` — so this
   fix wasn't disabling something libc++ normally has on; it confirms MSYS2 specifically
   packaged their `libc++` build with a non-default, elevated hardening level. Also found
   why `-DNDEBUG` alone didn't already handle this: libc++'s hardening/assertion mechanism is
   explicitly independent of `NDEBUG` (unlike `assert()` or MSVC's iterator-debug level) —
   there's an open FreeBSD toolchain bug about exactly this surprise (see Sources).
2. Read libc++'s current `vector::erase()` source directly
   (`libcxx/include/__vector/vector.h`) — the tail-shift is a plain, unguarded
   `std::move(__p + 1, __layout_.__end_ptr(), __p)` call, with no branch on
   `is_trivially_copyable`/relocatability. libstdc++ (`__copy_move` dispatch in
   `<bits/stl_algobase.h>`) and MSVC STL (`_Memmovable`-gated paths in `<xutility>`) are both
   long-documented to special-case exactly this shape — a left-shift over an overlapping,
   trivially-copyable range — with a bulk `memmove` instead of a per-element move-assignment
   loop.

Not confirmed with full certainty: whether libc++'s underlying `std::move` *algorithm*
implementation has an equivalent memmove dispatch that simply isn't triggering for this call
shape, versus genuinely lacking one — would need disassembly or an isolated micro-benchmark
to nail down definitively. The observed magnitude is consistent with a genuine per-element
move-assignment loop.

**Relevance:** this is a real, citable answer to RQ3.1 ("How do standard library
implementations impact `std::flat_map`'s performance relative to the sparse set and
alternative map data structures?"), not a benchmark artifact — vector-backed flat maps
(`std::flat_map` and the SG14 reference `flat_map` alike) can be dramatically slower under
libc++ specifically for erase-heavy workloads, traceable to a concrete, inspectable
source-level difference rather than an unexplained anomaly. Good candidate for a follow-up
isolated micro-benchmark (bare `std::vector<std::pair<uint32_t,float>>::erase` across all
three libraries, stripped of the flat_map/ECS wrapper) in the next phase, to confirm the
mechanism definitively and reconfirm at full (1,000,000-element) scale.

## Sources

- P0429R9, *A Standard `flat_map`* — https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0429r9.pdf
- P0447R28, *Introduction of `std::hive` to the standard library* — https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p0447r28.html
- `plf::hive` (zlib license, Matthew Bentley) — https://plflib.org/ — vendored at `libs/plf_hive`
- SG14 reference `flat_map` (Boost Software License) — https://github.com/WG21-SG14/SG14 — vendored at `libs/SG14`
- `SparseSet` (own implementation) — https://github.com/MauroDeryckere/SparseSet — vendored at `libs/SparseSet`
- cppreference, *Compiler support for C++23* (flat_map/flat_set row) — https://en.cppreference.com/w/cpp/compiler_support/23.html
- cppreference, *Compiler support for C++26* (`<hive>` row) — https://en.cppreference.com/w/cpp/compiler_support/26.html
- mingw-w64.org, *Windows / MSYS2 (Clang/LLVM)* — https://www.mingw-w64.org/getting-started/msys2-llvm/
- WinLibs GCC+MinGW-w64 builds — https://winlibs.com/
- LLVM release binaries — https://github.com/llvm/llvm-project/releases
- MSYS2 packages — https://packages.msys2.org/
- libc++ `vector::erase` source — https://github.com/llvm/llvm-project/blob/main/libcxx/include/__vector/vector.h
- libc++ Hardening Modes documentation — https://libcxx.llvm.org/Hardening.html
- P1144, *Object relocation in terms of move plus destroy* (trivial relocation background) — https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html
- Arthur O'Dwyer, *STL algorithms for trivial relocation* — https://quuxplusone.github.io/blog/2023/03/03/relocate-algorithm-design/
- FreeBSD toolchain list, *LIBCPP assertions are enabled in optimized builds when -DNDEBUG is given to clang* — https://www.mail-archive.com/toolchain@freebsd.org/msg01802.html
- Microsoft Learn, */GS (Buffer Security Check)* — https://learn.microsoft.com/en-us/cpp/build/reference/gs-buffer-security-check
- Preshing on Programming, *The Cost of Buffer Security Checks in Visual C++* — https://preshing.com/20110807/the-cost-of-buffer-security-checks-in-visual-c/
- MSYS2/MINGW-packages issue #4672, *Segfault when -fstack-protector enabled on simple programs on mingw64-gcc* (context on mingw-w64 stack-protector defaults) — https://github.com/msys2/MINGW-packages/issues/4672
- Microsoft Learn, */guard (Enable Control Flow Guard)* — https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard
- Microsoft Learn, */arch (x64)* — https://learn.microsoft.com/en-us/cpp/build/reference/arch-x64
