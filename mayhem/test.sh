#!/usr/bin/env bash
#
# mayhem/test.sh — RUN the behavioral KAT oracle built by mayhem/build.sh.
#
# Upstream qoiview ships NO test suite (it is a GUI viewer), so the oracle is the
# authored known-answer test /mayhem/qoi_kat_test: golden pixel-checksum decodes of
# the reference images upstream ships, encode/decode round-trip identity, and
# corrupted-input rejection. It prints one PASS/FAIL line per assertion and a final
# "KAT_RESULT passed=<p> failed=<f>" summary; a neutered (exit-0 no-op) binary
# produces no summary and is counted as a failure here.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

RUNNER="$SRC/qoi_kat_test"
if [ ! -x "$RUNNER" ]; then
  echo "FATAL: $RUNNER missing — mayhem/build.sh should have built it" >&2
  emit_ctrf "qoi-kat" 0 1
  exit 1
fi

out="$("$RUNNER" 2>&1)"; rc=$?
echo "$out"

summary="$(echo "$out" | grep -E '^KAT_RESULT passed=[0-9]+ failed=[0-9]+$' | tail -1)"
if [ -z "$summary" ]; then
  # No summary line (e.g. the binary was neutered to a no-op) — that is a failure.
  echo "FATAL: qoi_kat_test produced no KAT_RESULT summary (rc=$rc)" >&2
  emit_ctrf "qoi-kat" 0 1
  exit 1
fi

passed="$(echo "$summary" | sed -E 's/.*passed=([0-9]+).*/\1/')"
failed="$(echo "$summary" | sed -E 's/.*failed=([0-9]+).*/\1/')"
[ "$rc" -ne 0 ] && [ "$failed" -eq 0 ] && failed=1   # nonzero exit with clean counts is still a failure

emit_ctrf "qoi-kat" "$passed" "$failed"
