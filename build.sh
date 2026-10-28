#!/usr/bin/env bash
# Build the Linux x86-64 release into out/build/linux-amd64-release.
# The first configure downloads SDOJ-Recomp (for the ReXGlue SDK) and
# applies the patches in sdk-patches.
set -euo pipefail
cd "$(dirname "$0")"

cmake --preset linux-amd64-release "$@"
cmake --build --preset linux-amd64-release --parallel
mkdir -p out/build/linux-amd64-release/game_data
echo "Built. Put the game files in out/build/linux-amd64-release/game_data"
