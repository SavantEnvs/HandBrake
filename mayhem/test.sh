#!/usr/bin/env bash
#
# mayhem/test.sh — RUN HandBrake's functional checks (binaries already built by mayhem/build.sh).
# exit 0 = pass. EDIT per repo. PATCH-grade oracle: after an agent patches the source, the grader
# rebuilds (build.sh) then runs this. DELETE this file if the repo has no meaningful tests.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
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

# Known-answer checks over libhb (hb_kat) and end-to-end checks through upstream's HandBrakeCLI:
# scan a generated Y4M, transcode it to Matroska with x264, then scan the result.
for b in "$SRC/hb_kat" "$SRC/HandBrakeCLI"; do
  [ -x "$b" ] || { echo "missing $b — mayhem/build.sh should have built it" >&2; emit_ctrf "handbrake-kat" 0 1; exit 1; }
done

passed=0; failed=0
check() {  # check <name> <actual> <expected-substring>
  if [[ "$2" == *"$3"* ]]; then passed=$((passed+1)); echo "PASS $1"
  else failed=$((failed+1)); echo "FAIL $1: expected '$3' in: ${2:0:300}" >&2; fi
}

WORK="$(mktemp -d "${TMPDIR:-/tmp}/hb-test.XXXXXX")"
trap 'rm -rf "$WORK"' EXIT

kat="$("$SRC/hb_kat" 2>/dev/null | tr -d ' \n')"
check nal_annexb_to_mp4 "$kat" 'annexb_to_mp4=0000000465aabbcc000000024111'
check nal_mp4_to_annexb "$kat" 'mp4_to_annexb=0000000165aabbcc000000014111'
check filter_parse "$kat" 'filter_dict={"mode":"3","parity":"1"}'
check filter_string "$kat" 'filter_string=mode=3:parity=1'
check json_roundtrip "$kat" 'json_roundtrip={"a":[1,2,{"b":"c"}],"n":2.5}'
check builtin_presets_import "$kat" 'has_fast1080=1'
check preset_import "$kat" 'preset_name=katquality=21.0format=av_mkvaudio_fallback=ac3'
check reduce "$kat" 'reduce=16/9'
check encoder_names "$kat" 'encoder_x264=1name=H.264(x264)'
check samplerate_name "$kat" 'samplerate_48000=48'
check container_name "$kat" 'container_mkv=Matroska(avformat)'

# 10 frames of 64x48 4:2:0 at 30 fps.
{
  printf 'YUV4MPEG2 W64 H48 F30:1 Ip A1:1 C420jpeg\n'
  for i in 1 2 3 4 5 6 7 8 9 10; do
    printf 'FRAME\n'
    head -c 4608 /dev/zero | tr '\0' "\\$(printf '%03o' $((i * 20)))"
  done
} > "$WORK/in.y4m"

cli() { timeout 300 "$SRC/HandBrakeCLI" "$@" 2>&1; }

scan="$(cli -i "$WORK/in.y4m" --scan)"
check cli_scan_title "$scan" 'scan thread found 1 valid title(s)'
check cli_scan_geometry "$scan" 'size: 64x48, pixel aspect: 1/1, display aspect: 1.33, 30.000 fps'

enc="$(cli -i "$WORK/in.y4m" -o "$WORK/out.mkv" -f av_mkv -e x264 -q 20 --encoder-preset ultrafast)"
check cli_encode_frames "$enc" 'mux: track 0, 10 frames'
check cli_encode_done "$enc" 'Encode done!'

scan2="$(cli -i "$WORK/out.mkv" --scan)"
check cli_rescan_codec "$scan2" 'Video: h264'
check cli_rescan_geometry "$scan2" 'size: 64x48, pixel aspect: 1/1, display aspect: 1.33, 30.000 fps'

presets="$(cli --preset-list)"
check cli_preset_list "$presets" 'Very Fast 1080p30'

emit_ctrf "handbrake-kat" "$passed" "$failed"
