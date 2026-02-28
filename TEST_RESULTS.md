# Test Results Log

Running record for regression tracking across iterations.

## Best Known Results (So Far)

| Suite | Config | Total | Passed | Failed | Skipped | Status | Last Updated |
|---|---|---:|---:|---:|---:|---|---|
| picolibc gtest | `PICOLIBC_OPT_LEVEL=2` | 181 | 162 | 0 | 19 | PASS | 2026-02-25 21:27 |
| picolibc gtest | default (`PICOLIBC_OPT_LEVEL=1`) | 181 | 162 | 0 | 19 | PASS | 2026-02-26 01:40 |
| picolibc gtest | `--no-rebuild` baseline | 181 | 162 | 0 | 19 | PASS | 2026-02-23 00:35 |
| m65832 c_tests | `./run_c_tests.sh all` | 193 | 193 | 0 | 0 | PASS | 2026-02-22 |
| llvm test-suite (emulator) | `medium` preset (`SingleSource/Regression/C`, `SingleSource/UnitTests`, selected `MultiSource/Benchmarks/*`) | 230 | 118 | 33 | 79 (`Executable Missing`) | PARTIAL | 2026-02-26 23:20 |
| picolibc stdio trio | `test-fopen/test-mktemp/test-setvbuf`, opt `0/1/2/3` | 3 | 3 | 0 | 0 | PASS | 2026-02-22 |

## LLVM Upstream High-Water Mark

| Suite | Scope | Total | Passed | Failed | Skipped | Status | Last Updated | Artifact |
|---|---|---:|---:|---:|---:|---|---|---|
| `llvm-lit` (`llvm/test`) | full (`check-llvm`, host-enabled build) | 71275 | 32262 | 0 | 224 | PASS | 2026-02-25 22:55 | `test-results/llvm-lit/check_llvm_host_20260225_225500.txt` |
| `llvm-lit` (`llvm/test/CodeGen/M65832`) | target-only | 2 | 2 | 0 | 0 | PASS | 2026-02-25 22:27 | `test-results/llvm-lit/check_llvm_codegen_m65832_20260225_222745.txt` |
| `llvm/test/LTO/empty-triple.ll` | direct RUN pipeline (`llvm-as` + `llvm-lto` + `FileCheck`) on host-enabled build | 1 | 1 | 0 | 0 | PASS | 2026-02-25 22:47 | `test-results/llvm-lit/retest_empty_triple_hosttarget_direct_20260225_224753.txt` |
| `llvm-lit` (`clang/test`) | full (`check-clang`, host-enabled build) | 50260 | 45734 | 0 | 15 | PASS | 2026-02-25 23:15 | `test-results/llvm-lit/check_clang_host_20260225_231549.txt` |
| Cross-project (`compiler-rt` / `libc++`) | full | n/a | n/a | n/a | n/a | NOT RUN YET | n/a | n/a |

## Iteration History

| Timestamp | Suite | Command | Build Opt | Total | Passed | Failed | Skipped | Duration | LLVM | EMU | PICOLIBC HARNESS | Artifacts | Notes |
|---|---|---|---:|---:|---:|---:|---:|---|---|---|---|---|---|
| 2026-02-22 | m65832 c_tests (full) | `./run_c_tests.sh all` | n/a | 193 | 193 | 0 | 0 | n/a | `32e3af826` | `35ae0aa` | n/a | n/a | Full emulator c test sweep at standard suite config |
| 2026-02-22 | picolibc stdio targeted | targeted trio (`test-fopen`, `test-mktemp`, `test-setvbuf`) | 0/1/2/3 | 3 | 3 | 0 | 0 | n/a | `32e3af826` | `35ae0aa` | `52e6b7dd2` | n/a | Confirmed passing across all picolibc optimization levels after syscall flag translation fix |
| 2026-02-23 00:35 | picolibc gtest (full) | `python3 run_picolibc_gtest.py --no-rebuild` | baseline | 181 | 162 | 0 | 19 | n/a | `32e3af826` | `35ae0aa` | `52e6b7dd2` | `picolibc-m65832/test-results/results_20260223_003508.txt` | Full run at standard config; expected environment skips remain |
| 2026-02-25 21:27 | picolibc gtest (full) | `PICOLIBC_OPT_LEVEL=2 python3 run_picolibc_gtest.py` | 2 | 181 | 162 | 0 | 19 | 86.27s | `32e3af826` | `35ae0aa` | `52e6b7dd2` | `picolibc-m65832/test-results/results_20260225_212700.txt` | Full rebuild + full run; stdio cases pass (`test-fopen`, `test-mktemp`, `test-setvbuf`) |
| 2026-02-25 21:48-22:19 | llvm upstream (`check-llvm`) | `cmake --build build-lit --target check-llvm -j8` | n/a | 71153 | 23781 | 2 | 696 | 1839.58s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/check_llvm_20260225_214822.txt` | Also reports `Unsupported=46632`, `Expectedly Failed=42`; fails: `CodeGen/M65832/load-zext.ll`, `LTO/empty-triple.ll` |
| 2026-02-25 22:22 | llvm target (`CodeGen/M65832`) | `build-lit/bin/llvm-lit -sv build-lit/test/CodeGen/M65832` | n/a | 2 | 1 | 1 | 0 | 0.27s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/check_llvm_codegen_m65832_20260225_222205.txt` | Fail: `CodeGen/M65832/load-zext.ll` |
| 2026-02-25 22:27 | llvm retest (prior 2 failures) | `build-lit/bin/llvm-lit -sv build-lit/test/CodeGen/M65832/load-zext.ll build-lit/test/LTO/empty-triple.ll` | n/a | 2 | 1 | 1 | 0 | 0.74s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/retest_two_failures_20260225_222715.txt` | `load-zext.ll` fixed; remaining fail is `LTO/empty-triple.ll` |
| 2026-02-25 22:27 | llvm target (`CodeGen/M65832`) | `build-lit/bin/llvm-lit -sv build-lit/test/CodeGen/M65832` | n/a | 2 | 2 | 0 | 0 | 0.26s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/check_llvm_codegen_m65832_20260225_222745.txt` | Target-specific suite now fully passing |
| 2026-02-25 22:47 | llvm LTO single-test retest | direct test RUN pipeline: `llvm-as < llvm/test/LTO/empty-triple.ll` then `llvm-lto ... | FileCheck` on `build-lit-host` | n/a | 1 | 1 | 0 | 0 | n/a | `32e3af826` | n/a | n/a | `test-results/llvm-lit/retest_empty_triple_hosttarget_direct_20260225_224753.txt` | Passes when build includes host target (`AArch64`) + experimental `M65832`; previous failure was build-target configuration dependent |
| 2026-02-25 22:55 | llvm upstream (`check-llvm`) | `cmake --build build-lit-host --target check-llvm -j8` | n/a | 71275 | 32262 | 0 | 224 | 854.39s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/check_llvm_host_20260225_225500.txt` | Clean run (exit 0); also reports `Unsupported=38740`, `Expectedly Failed=49` |
| 2026-02-25 23:15 | clang upstream (`check-clang`) | `cmake --build build-lit-host --target check-clang -j8` | n/a | 50260 | 45734 | 0 | 15 | 1764.85s | `32e3af826` | n/a | n/a | `test-results/llvm-lit/check_clang_host_20260225_231549.txt` | Clean run (exit 0); also reports `Unsupported=4484`, `Expectedly Failed=27` |
| 2026-02-26 00:36 | llvm test-suite (`SingleSource/Regression/C`) | `build-fast/bin/llvm-lit -sv -j8 SingleSource/Regression/C` | n/a | 36 | 25 | 10 | 1 (`Executable Missing`) | 0.93s | working tree | observed live | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/fix_regression_c_after_signedcmp_20260226_003637.txt` | Signed compare lowering fix landed; `Regression-C-compare` moved to pass |
| 2026-02-26 00:46 | llvm test-suite (`SingleSource/Regression/C`) | `build-fast/bin/llvm-lit -sv -j8 SingleSource/Regression/C` | n/a | 36 | 25 | 11 | 0 | 1.47s | working tree | observed live | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/regression_c_after_brind_20260226_004631.txt` | Added `BRIND` selection support: removed compile-time `NOEXE` for `2004-03-15-IndirectGoto` (now runtime fail) |
| 2026-02-26 00:47 | llvm test-suite emulator expansion (`medium` preset) | `build-fast/bin/llvm-lit -sv -j8 SingleSource/Regression/C SingleSource/UnitTests MultiSource/Benchmarks/{Prolangs-C,MallocBench,MiBench,Olden,FreeBench}` | n/a | 230 | 110 | 40 | 80 (`Executable Missing`) | 2.87s | working tree | observed live (`pgrep` PIDs logged) | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/expanded_c_medium_20260226_004736.txt` | First low-effort high-yield runtime expansion beyond SingleSource-only scope |
| 2026-02-26 01:05 | llvm test-suite (`SingleSource/Regression/C`) | `scripts/run_m65832_testsuite_emulator.sh --preset regression-c --jobs 8` | n/a | 36 | 26 | 10 | 0 | 2.56s | working tree | observed in focused retest (PID log), sampled run too short for preset poll | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/emulator_regression-c_20260226_010503.txt` | `Regression-C-2004-03-15-IndirectGoto` now passes after indirect-branch DP-slot sync fix |
| 2026-02-26 01:38 | picolibc gtest (targeted regressions) | `python3 run_picolibc_gtest.py <26 previously failing tests>` | 1 | 26 | 26 | 0 | 0 | 36.54s | working tree | observed live (per-test emulator invocations) | `run_picolibc_gtest.py` | `picolibc-m65832/test-results/results_20260226_013859.txt` | Restored regressions by removing extra pre-store in `JSR_IND` expansion; indirect calls now stable in stdio/string/time/fnmatch paths |
| 2026-02-26 01:40 | picolibc gtest (full) | `python3 run_picolibc_gtest.py` | 1 | 181 | 162 | 0 | 19 | 81.36s | working tree | observed live (per-test emulator invocations) | `run_picolibc_gtest.py` | `picolibc-m65832/test-results/results_20260226_014048.txt` | Full sanity restored at default opt level after `JSR_IND` lowering fix |
| 2026-02-26 01:41 | m65832 c_tests (full) | `./run_c_tests.sh all` | n/a | 193 | 193 | 0 | 0 | 20.54s | working tree | observed live | n/a | n/a | Post-fix regression sweep remains clean |
| 2026-02-26 01:40 | llvm test-suite (`regression-c` preset) | `scripts/run_m65832_testsuite_emulator.sh --preset regression-c --jobs 8` | n/a | 36 | 26 | 10 | 0 | 2.84s | working tree | not observed (run too short for poll window) | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/emulator_regression-c_20260226_014054.txt` | Failure set unchanged vs prior run; varargs trio still fails when linked via test-suite/sysroot pipeline |
| 2026-02-26 18:02 | picolibc gtest (full) | `python3 run_picolibc_gtest.py` | 1 | 181 | 162 | 0 | 19 | 101.94s | working tree | observed live (per-test emulator invocations) | `run_picolibc_gtest.py` | `picolibc-m65832/test-results/results_20260226_180227.txt` | Full run after FPU pseudo clobber modeling; still clean (0 fails) |
| 2026-02-26 19:10 | llvm test-suite emulator expansion (`medium` preset) | `scripts/run_m65832_testsuite_emulator.sh --preset medium --jobs 8` | n/a | 230 | 114 | 36 | 80 (`Executable Missing`) | 4.87s | working tree | observed via PID snapshots file | `test-suite-m65832/m65832-run.sh` | `test-results/llvm-lit/emulator_medium_20260226_191008.txt` | Improved vs prior medium run (+4 pass / -4 fail); varargs-related regressions removed from failing set |
| 2026-02-26 22:25 | llvm test-suite emulator expansion (`medium` preset) | `scripts/run_m65832_testsuite_emulator.sh --preset medium --jobs 8` | n/a | 230 | 115 | 35 | 80 (`Executable Missing`) | 7.71s | working tree | observed live | `test-suite-m65832/m65832-run.sh` | n/a | Sysroot sync: `run_picolibc_gtest.py` now installs libc.a/libsys.a to m65832-sysroot after rebuild; `Popcount-ffs-fls` passes with synced libc |
| 2026-02-26 23:15 | llvm test-suite emulator expansion (`medium` preset) | `scripts/run_m65832_testsuite_emulator.sh --preset medium --jobs 8` | n/a | 230 | 119 | 32 | 79 (`Executable Missing`) | 5.45s | working tree | observed live | `test-suite-m65832/m65832-run.sh` | n/a | Byval struct fix; vararg 8-byte align (reverted — breaks picolibc; 4-byte is final) |
| 2026-02-26 23:20 | llvm test-suite emulator expansion (`medium` preset) | `scripts/run_m65832_testsuite_emulator.sh --preset medium --jobs 8` | n/a | 230 | 118 | 33 | 79 (`Executable Missing`) | n/a | working tree | n/a | `test-suite-m65832/m65832-run.sh` | n/a | Reverted vararg 8-byte align; byval fix kept. float_varg fails again (4-byte align is ABI) |
| 2026-02-26 19:16 | llvm test-suite (varargs trio, focused retest) | `build-fast/bin/llvm-lit -sv -j1 SingleSource/Regression/C/Regression-C-callargs.test SingleSource/UnitTests/2003-05-07-VarArgs.test SingleSource/UnitTests/2003-08-11-VaListArg.test` | n/a | 3 | 3 | 0 | 0 | 0.78s | working tree | observed live (single-worker emulator run) | `test-suite-m65832/m65832-run.sh` | n/a (console run) | All previously failing varargs trio tests now pass |

## Update Rule

For each new run, append one row to **Iteration History** and update **Best Known Results** (including LLVM high-water rows) only when pass/fail metrics improve or tie with better coverage/runtime.

## Emulator Validity Rule

For any row claimed as emulator-backed (cross-target execution), all of the following must be true:

1. The run uses an emulator protocol adapter/wrapper (not raw emulator output).
2. The wrapper maps emulator output to canonical test status and filtered stdout.
3. A live `m65832emu` process is observed during execution.

Current adapter source of truth:

- `m65832-stdlib/picolibc/run-m65832.sh`
- `m65832-stdlib/picolibc/cross-m65832.txt` (`exe_wrapper`)

If `m65832emu` is not running during the test window, the run must not be logged as emulator-backed.

## Data Source

Rows above are from already completed runs (historical logs/transcript + saved artifacts), not newly re-executed tests in this update.
