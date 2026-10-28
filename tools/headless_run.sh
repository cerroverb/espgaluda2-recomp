#!/usr/bin/env bash
# Runs the game in a headless gamescope on the real GPU and takes screenshots with gamescopectl.
# Original code (WORKLOG.md "Original code"): the test harness for this machine (no display).
#   tools/headless_run.sh <build dir> <out prefix> <seconds to run> "<shot times, seconds>" [game args...]
# Screenshots: <out prefix>_<t>s.png; gamescope output: <out prefix>_gamescope.log.
set -u
build=$1; prefix=$(realpath -m "$2"); total=$3; shots=$4; shift 4
mkdir -p "$(dirname "$prefix")"
cd "$build" || exit 1
pactl list short sinks | grep -q espgaluda_sink || pactl load-module module-null-sink sink_name=espgaluda_sink >/dev/null
pkill -x gamescope; pkill -9 -x espgaluda2_reco; sleep 1
setsid gamescope --backend headless -W 1280 -H 720 -w 1280 -h 720 -r 60 -- ./espgaluda2_recomp "$@" \
  > "${prefix}_gamescope.log" 2>&1 &
export GAMESCOPE_WAYLAND_DISPLAY=gamescope-0
start=$(date +%s.%N)
for t in $shots; do
  now=$(date +%s.%N); wait=$(python3 -c "print(max(0,$start+$t-$now))"); [ "${wait:0:1}" != "-" ] && sleep "$wait"
  gamescopectl screenshot "${prefix}_${t}s.png" >/dev/null 2>&1
done
now=$(date +%s.%N); wait=$(python3 -c "print(max(0,$start+$total-$now))"); [ "${wait:0:1}" != "-" ] && sleep "$wait"
alive=$(pgrep -x espgaluda2_reco >/dev/null && echo yes || echo no)
pkill -x gamescope; sleep 1; pkill -9 -x espgaluda2_reco
echo "game still running at the end: $alive"
