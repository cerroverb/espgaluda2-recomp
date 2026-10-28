// Ported from github.com/ChanseyIsTheBest/futari-nx (pull request 2, commit 7f5a36a), app/src/renut_engine/gpu/banjo_renut_checks.cpp. GPL-3.0 (LICENSE-GPL-3.0).
// banjo-nx (package 27c): QueueKickFlush. See banjo_renut_checks.h.
#include "renut_engine/gpu/banjo_renut_checks.h"

#include "renut_engine/gpu/backend/render_thread.h"
#include "renut_engine/gpu/render/draw.h"
#include "renut_engine/gpu/render/textures.h"
#include "renut_engine/gpu/backend/video.h"

namespace banjo::renut_gpu::checks {

void QueueKickFlush() {
  ::renut::gpu::render_thread::Queue([] {
    ::renut::gpu::FlushVertexFillsLocked();
    ::renut::gpu::FlushTextureFillsLocked();
    ::renut::gpu::Video::KickLocked();  // banjo-nx (2026-10-19): the ring cleaner and the kick submits
  });
}

}  // namespace banjo::renut_gpu::checks
