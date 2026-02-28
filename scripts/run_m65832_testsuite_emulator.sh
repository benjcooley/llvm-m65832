#!/usr/bin/env bash
set -euo pipefail

ROOT="/Users/benjamincooley/projects/llvm-m65832"
TS_BUILD="/Users/benjamincooley/projects/test-suite-build-m65832"
LIT_BIN="$ROOT/build-fast/bin/llvm-lit"

usage() {
  cat <<'EOF'
Usage:
  run_m65832_testsuite_emulator.sh [--preset PRESET] [--jobs N] [--configure]

Presets:
  regression-c  SingleSource/Regression/C only
  small         Regression/C + UnitTests
  medium        C-focused expanded runtime subset (SingleSource + MultiSource)
  large         Medium + additional C-focused MultiSource directories

Notes:
  - This script runs emulator-backed test-suite paths only.
  - It captures lit output and periodic m65832emu PID snapshots.
EOF
}

PRESET="medium"
JOBS=8
CONFIGURE=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --preset)
      PRESET="${2:-}"
      shift 2
      ;;
    --jobs)
      JOBS="${2:-}"
      shift 2
      ;;
    --configure)
      CONFIGURE=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ ! -x "$LIT_BIN" ]]; then
  echo "Missing llvm-lit binary: $LIT_BIN" >&2
  exit 2
fi

TS="$(date +%Y%m%d_%H%M%S)"
OUT_DIR="$ROOT/test-results/llvm-lit"
mkdir -p "$OUT_DIR"
OUT_FILE="$OUT_DIR/emulator_${PRESET}_${TS}.txt"
PID_LOG="$OUT_DIR/emulator_${PRESET}_${TS}_pids.txt"

declare -a PATHS
case "$PRESET" in
  regression-c)
    PATHS=("SingleSource/Regression/C")
    ;;
  small)
    PATHS=(
      "SingleSource/Regression/C"
      "SingleSource/UnitTests"
    )
    ;;
  medium)
    PATHS=(
      "SingleSource/Regression/C"
      "SingleSource/UnitTests"
      "MultiSource/Benchmarks/Prolangs-C"
      "MultiSource/Benchmarks/MallocBench"
      "MultiSource/Benchmarks/MiBench"
      "MultiSource/Benchmarks/Olden"
      "MultiSource/Benchmarks/FreeBench"
    )
    ;;
  large)
    PATHS=(
      "SingleSource/Regression/C"
      "SingleSource/UnitTests"
      "MultiSource/Benchmarks/Prolangs-C"
      "MultiSource/Benchmarks/MallocBench"
      "MultiSource/Benchmarks/MiBench"
      "MultiSource/Benchmarks/Olden"
      "MultiSource/Benchmarks/FreeBench"
      "MultiSource/Benchmarks/SciMark2-C"
      "MultiSource/Benchmarks/DOE-ProxyApps-C"
      "MultiSource/Applications/sqlite3"
      "MultiSource/Applications/lua"
    )
    ;;
  *)
    echo "Unknown preset '$PRESET'" >&2
    usage
    exit 2
    ;;
esac

if [[ "$CONFIGURE" -eq 1 ]]; then
  cmake -S "$TS_BUILD" -B "$TS_BUILD" \
    -DTEST_SUITE_RUN_TYPE=test \
    -DTEST_SUITE_SUBDIRS="SingleSource;MultiSource"
fi

echo "preset=$PRESET"
echo "jobs=$JOBS"
echo "artifact=$OUT_FILE"
echo "pid_log=$PID_LOG"
echo "paths=${PATHS[*]}"

(
  cd "$TS_BUILD"
  "$LIT_BIN" -sv -j"$JOBS" "${PATHS[@]}"
) | tee "$OUT_FILE" &
RUN_PID=$!

{
  echo "# m65832emu PID snapshots"
  echo "# started_at=$(date -Iseconds)"
} > "$PID_LOG"

while kill -0 "$RUN_PID" 2>/dev/null; do
  {
    echo "## $(date -Iseconds)"
    pgrep -af m65832emu || true
  } >> "$PID_LOG"
  sleep 2
done

set +e
wait "$RUN_PID"
RUN_STATUS=$?
set -e

if ! grep -Eq "^[0-9]" "$PID_LOG"; then
  echo "WARNING: no live m65832emu observed during run" | tee -a "$PID_LOG"
fi

echo "exit_code=$RUN_STATUS" | tee -a "$PID_LOG"
exit "$RUN_STATUS"
