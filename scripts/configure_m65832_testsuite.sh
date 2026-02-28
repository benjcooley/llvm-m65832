#!/usr/bin/env bash
set -euo pipefail

TS_BUILD="/Users/benjamincooley/projects/test-suite-build-m65832"

cmake -S "$TS_BUILD" -B "$TS_BUILD" \
  -DTEST_SUITE_RUN_TYPE=test \
  -DTEST_SUITE_SUBDIRS="SingleSource;MultiSource"

echo "Configured $TS_BUILD for emulator-backed expansion:"
echo "  TEST_SUITE_RUN_TYPE=test"
echo "  TEST_SUITE_SUBDIRS=SingleSource;MultiSource"
