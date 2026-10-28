#!/usr/bin/env bash
# Original code (WORKLOG.md "Clean exit"): kills the headless gamescope under a running game (Wayland) and reports
# whether and how fast the game exits.  tools/compositor_loss_test.sh <build dir> child|outside <seconds> [args]
#   child:   the game is gamescope's child on Wayland (--expose-wayland; gamescopereaper also signals it)
#   child-x11: the game is gamescope's child on Xwayland (gamescope's default, as in Steam's gaming mode)
#   outside: the game is started separately and only connects to gamescope-0 (as with a desktop compositor)
set -u
build=$(realpath "$1"); mode=$2; after=$3; shift 3
cd "$build" || exit 1
gs() { pgrep -f '^gamescope --backend'; }
kill -9 $(gs) 2>/dev/null; pkill -9 -x espgaluda2_reco; sleep 1; rm -f /dev/shm/xenia_memory_*
if [ "$mode" = child ]; then
  setsid gamescope --backend headless --expose-wayland -W 1280 -H 720 -w 1280 -h 720 -r 60 -- ${GAME:-./espgaluda2_recomp} \
    --video_driver=wayland "$@" >/dev/null 2>&1 &
elif [ "$mode" = child-x11 ]; then
  setsid gamescope --backend headless -W 1280 -H 720 -w 1280 -h 720 -r 60 -- ${GAME:-./espgaluda2_recomp} "$@" >/dev/null 2>&1 &
else
  setsid gamescope --backend headless -W 1280 -H 720 -w 1280 -h 720 -r 60 -- sleep 600 >/dev/null 2>&1 &
  sleep 3
  WAYLAND_DISPLAY=gamescope-0 DISPLAY= setsid ./espgaluda2_recomp --video_driver=wayland "$@" >/dev/null 2>&1 &
fi
sleep "$after"
pid=$(pgrep -x espgaluda2_reco); [ -z "$pid" ] && { echo "game not running"; exit 1; }
t0=$(date +%s.%N); kill -9 $(gs)
for i in $(seq 1 200); do kill -0 "$pid" 2>/dev/null || break; sleep 0.1; done
t1=$(date +%s.%N)
if kill -0 "$pid" 2>/dev/null; then echo "$mode: still running 20 s after gamescope died (killed)"; kill -9 "$pid"; else
  echo "$mode: exited $(python3 -c "print(round($t1-$t0,2))") s after gamescope died"; fi
sleep 1; rm -f /dev/shm/xenia_memory_*
L=$(ls -t logs/*.log | head -1); grep -E 'Shutting down|Execution complete|cache saved|Wayland|error|fatal|abort' "$L" | tail -6 | cut -c25-200
