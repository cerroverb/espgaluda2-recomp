// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/shaders/host/depth_resolve_ms_ps.hlsl. GPL-3.0 (LICENSE-GPL-3.0).
// Depth resolve from a multisampled depth target: sample 0, as the Xenos
// resolves depth (see depth_resolve.hlsli).
#define DEPTH_RESOLVE_MS 1
#include "depth_resolve.hlsli"
