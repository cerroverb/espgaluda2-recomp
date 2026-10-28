// espgaluda2-recomp: what Renut's renderer (as futari-nx carries it) takes from the rest of futari-nx, which this
// project does not have: futari-nx's src/native and src/futari_enhancements. Each piece is copied from futari-nx
// (github.com/ChanseyIsTheBest/futari-nx, pull request 2, commit 7f5a36a), GPL-3.0 (LICENSE-GPL-3.0); the path is
// given with each.
#pragma once

#include <cstdint>

namespace futari::native {

// From app/src/native/futari_native.h (CodeHash): FNV-1a 64 over the guest instruction words as stored (big-endian
// bytes). Only used on code (0x82xxxxxx), which has no physical host offset, so the translation is base + address.
inline uint64_t CodeHash(const uint8_t* base, uint32_t address, uint32_t words) {
  uint64_t h = 0xcbf29ce484222325ull;
  const uint8_t* p = base + address;
  for (uint32_t i = 0; i < words * 4; ++i) {
    h = (h ^ p[i]) * 0x100000001b3ull;
  }
  return h;
}

// futari-nx's codegen self-check report (app/src/native): this project's generated code has no such checks.
inline void CodegenChecksReport(int /*seconds*/) {}

}  // namespace futari::native

namespace futari::enhancements {

// From app/src/futari_enhancements.h: the two enhancement groups. futari-nx turns every enhancement on unless its
// group master is off (futari_cpu_enhancements / futari_noncpu_enhancements, both on by default); this project has no
// group masters, so both groups are on and each enhancement's own key decides.
enum class Group { kCpu, kNonCpu };
inline bool GroupOn(Group) { return true; }

}  // namespace futari::enhancements
