// espgaluda2-recomp (2026-10-30, original code): the English translation patch (the official English text of the
// game's Switch release in place of the Xbox 360 game's Japanese menus, messages and tutorials). See translation.cpp.
#pragma once

#include <cstdint>

namespace rex::memory {
class Memory;
}

namespace espgaluda2::translation {

// Writes the English lines into the loaded default.xex image (before the game runs). Does nothing when
// the translation_patch setting is off or the table (translation/english.tsv beside the executable) is missing.
void Apply(rex::memory::Memory* memory, uint32_t image_base, uint32_t image_size);

// Whether the renderer should hand its uploads to EnglishTextureLabels (the translation_patch setting).
bool TextureLabelsEnabled();

// Called by the renderer, when TextureLabelsEnabled, with each square 512 / 1024 texel 4-byte texture it uploads
// (XXH3 of the texels): keeps the ASCII font's texels and draws the tutorial controller diagram's labels in English.
void EnglishTextureLabels(uint64_t hash, uint8_t* texels, uint32_t width, uint32_t height, uint32_t row_pitch);

}  // namespace espgaluda2::translation
