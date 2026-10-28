#!/usr/bin/env bash
# Launcher: runs the game from its own folder so game_data and user_data
# beside it are found. Extra arguments are passed through.
cd "$(dirname "$(readlink -f "$0")")"
exec ./espgaluda2_recomp "$@"
