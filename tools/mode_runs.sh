#!/usr/bin/env bash
# Original code (WORKLOG.md "Task F"): runs the game once per mode with its scripted pad route (tools/mode_scripts/),
# a fresh profile each, screenshots and the rate counter, and keeps each run's log beside the screenshots.
#   tools/mode_runs.sh <build dir> <out dir> <seconds> "<shot times>" <renderer: renut|xenos> <mode>... [-- game args]
set -u
build=$1; out=$2; secs=$3; shots=$4; renderer=$5; shift 5
modes=(); while [ $# -gt 0 ] && [ "$1" != "--" ]; do modes+=("$1"); shift; done
[ "${1:-}" = "--" ] && shift
here=$(cd "$(dirname "$0")" && pwd)
mkdir -p "$out"
for m in "${modes[@]}"; do
  script=$(cat "$here/mode_scripts/$m.txt")
  "$here/headless_run.sh" "$build" "$out/${renderer}_$m" "$secs" "$shots" --futari_input_script="$script" \
    --espgaluda2_rate_counter=true --espgaluda2_renderer="$renderer" "$@" > /dev/null 2>&1
  log=$(ls -t "$build"/logs/*.log | head -1); cp "$log" "$out/${renderer}_$m.log"
  rates=$(grep -oE '[0-9]+ game updates/s' "$log" | awk '{print $1}' | sort -n | uniq -c | tr '\n' ' ')
  echo "$renderer $m: updates/s histogram (count value): $rates"
done
