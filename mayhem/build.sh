#!/usr/bin/env bash
#
# mayhem/build.sh — build qoiview's fuzz harness + KAT test oracle.
#
# qoiview is a GUI .qoi viewer on top of the sokol headers; the fuzzable code is
# the single-header QOI codec (qoi.h) that parses untrusted .qoi files. We build:
#   1. /mayhem/qoiview-fuzz             libFuzzer harness over qoi_decode (sanitized)
#   2. /mayhem/qoiview-fuzz-standalone  run-once reproducer (same harness, standalone driver)
#   3. /mayhem/qoi_kat_test             behavioral KAT oracle (NORMAL flags; run by test.sh)
# The GUI binary itself (qoiview.c) needs X11/GL and is not built — no harness
# drives it and it isn't a fuzz target.
set -euo pipefail

# clang rejects SOURCE_DATE_EPOCH='' (empty) — it must be unset or a valid integer.
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

# Build knobs come from the ENVIRONMENT (overridable). SANITIZER_FLAGS uses `=` (no colon) so an
# explicit EMPTY value (--build-arg SANITIZER_FLAGS=) is honored and builds with NO sanitizers.
: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# 1) Fuzz harness (sanitized + instrumented; qoi.h is header-only so the codec under test
#    is compiled directly into the harness with $SANITIZER_FLAGS).
$CC $SANITIZER_FLAGS $DEBUG_FLAGS $LIB_FUZZING_ENGINE \
    "$SRC/mayhem/qoiview-fuzz.c" -I"$SRC" -o "$SRC/qoiview-fuzz"

# 2) Standalone run-once reproducer (same harness, non-fuzzer driver).
$CC $SANITIZER_FLAGS $DEBUG_FLAGS "$STANDALONE_FUZZ_MAIN" \
    "$SRC/mayhem/qoiview-fuzz.c" -I"$SRC" -o "$SRC/qoiview-fuzz-standalone"

# 3) KAT test oracle with the project's NORMAL flags (independent of the sanitized build)
#    so test.sh only RUNS it. $COVERAGE_FLAGS is empty by default.
$CC -O2 $COVERAGE_FLAGS "$SRC/mayhem/qoi_kat_test.c" -I"$SRC" -o "$SRC/qoi_kat_test"

echo "build.sh: built qoiview-fuzz, qoiview-fuzz-standalone, qoi_kat_test"
