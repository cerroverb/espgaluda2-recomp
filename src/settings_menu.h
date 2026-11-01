// Original code (2026-11-01). The F4 settings window lists every setting compiled into the program, most of them from
// the code this port reuses (the SDK's emulated GPU, banjo-nx's Switch renderer tuning, futari-nx). Apply() moves the
// settings that apply to Espgaluda II into the "Espgaluda2" category and marks every other one debug-only, which the
// window leaves out (sdk-patches/0013). Names do not change: config files and launch options work as before.

#pragma once

namespace espgaluda2::settings_menu {

// Call once every setting and key binding is registered (OnPostSetup).
void Apply();

}  // namespace espgaluda2::settings_menu
