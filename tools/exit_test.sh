#!/usr/bin/env bash
# Original code (WORKLOG.md "Clean exit"): starts the game in headless gamescope, sends a signal after N seconds and
# reports how long the game takes to exit and how it ended.  tools/exit_test.sh <build dir> <signal> <seconds> [args]
set -u
build=$(realpath "$1"); sig=$2; after=$3; shift 3
cd "$build" || exit 1
kill -9 $(pgrep -f '^gamescope --backend') 2>/dev/null; pkill -9 -x espgaluda2_reco; sleep 1; rm -f /dev/shm/xenia_memory_*
setsid gamescope --backend headless -W 1280 -H 720 -w 1280 -h 720 -r 60 -- ./espgaluda2_recomp "$@" >/dev/null 2>&1 &
sleep "$after"
pid=$(pgrep -x espgaluda2_reco); [ -z "$pid" ] && { echo "game not running"; exit 1; }
t0=$(date +%s.%N); kill -"$sig" "$pid"
for i in $(seq 1 200); do kill -0 "$pid" 2>/dev/null || break; sleep 0.1; done
t1=$(date +%s.%N)
if kill -0 "$pid" 2>/dev/null; then echo "SIG$sig: still running after 20 s (killed)"; kill -9 "$pid"; else
  echo "SIG$sig: exited after $(python3 -c "print(round($t1-$t0,2))") s"; fi
kill -9 $(pgrep -f '^gamescope --backend') 2>/dev/null; sleep 1; rm -f /dev/shm/xenia_memory_*
L=$(ls -t logs/*.log | head -1); grep -E 'Shutting down|error|fatal|abort' "$L" | tail -4 | cut -c25-200
