// espgaluda2-recomp (2026-10-30, original code): the English translation patch: the official English text of the game's Switch release
// (Live Wire / CAVE, 2021) in place of the Xbox 360 game's Japanese menus, messages and tutorials.
//
// translation/english.tsv (tools/translation/make_map.py, from the Switch release's out_nx_jp.btxt / out_nx_en.btxt)
// lists, per Japanese string of default.xex: its guest address, the bytes it may take in place, whether its address is
// in the image's data (a pointer table) and the English line, already in the Xbox 360 game's bytes (Shift-JIS, its
// button glyphs and colour codes). Before the game runs:
// - a line that fits is written over the Japanese one (the rest of its bytes zeroed);
// - a longer one whose address the image holds as a pointer is written to new guest memory and every such pointer
//   outside the code section is pointed at it; the Japanese one stays where it was;
// - every line also gets a copy in new guest memory, and the text drawer and measure hooks below swap a text pointer
//   equal to an original string's address for its copy (psxrecomp's "pointer swap" method, docs/STRING_TRANSLATION.md; technique only),
//   so lines reached only from code have no length limit either.
//
// The game's dialog text drawer (sub_820A16A8) and its line-width measure (sub_820A15A0) take two bytes per
// character and draw from the Japanese font pages only: a single-byte letter matches none of their cases and is
// skipped. For a line with letters the hooks draw it in runs: Japanese and button glyphs through the original drawer
// at the run's position; letters, digits and signs through the game's own ASCII string drawer sub_820A27A0 (the 16x32
// font of texture 10001, advance 17, 15 for ',' and '.'). The advances (33 a glyph, 16 a space, 33 a line) and the
// alignment (flags 1 left, 2 centre, 4 right) are the drawer's own. A line without letters takes the original path.
#include "translation.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/system/xmemory.h>

#include "generated/default/espgaluda2_recomp_init.h"

extern "C" REX_FUNC(__imp__sub_820A16A8);
extern "C" REX_FUNC(__imp__sub_820A15A0);
extern "C" REX_FUNC(sub_820A27A0);

REXCVAR_DEFINE_BOOL(translation_patch, true, "Espgaluda2",
                    "English translation patch: the menus, messages and tutorials in the official English of the game's "
                    "Switch release (off: the Xbox 360 original's Japanese). Applies at the next launch")
    .lifecycle(rex::cvar::Lifecycle::kInitOnly);

namespace espgaluda2::translation {
namespace {

struct Line {
  uint32_t address = 0;
  uint32_t capacity = 0;
  bool in_data = false;
  std::string id;
  std::vector<uint8_t> bytes;
};

std::vector<Line> Load(const std::filesystem::path& path) {
  std::vector<Line> lines;
  std::ifstream in(path);
  std::string row;
  while (std::getline(in, row)) {
    if (row.empty() || row[0] == '#') continue;
    std::istringstream fields(row);
    std::string address, capacity, in_data, id, hex;
    if (!std::getline(fields, address, '\t') || !std::getline(fields, capacity, '\t') ||
        !std::getline(fields, in_data, '\t') || !std::getline(fields, id, '\t') || !std::getline(fields, hex, '\t'))
      continue;
    Line line;
    line.address = uint32_t(std::stoul(address, nullptr, 16));
    line.capacity = uint32_t(std::stoul(capacity));
    line.in_data = in_data == "1";
    line.id = id;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) line.bytes.push_back(uint8_t(std::stoul(hex.substr(i, 2), nullptr, 16)));
    lines.push_back(std::move(line));
  }
  return lines;
}

// Original string address -> its English copy (guest addresses); the run scratch buffer.
std::unordered_map<uint32_t, uint32_t> g_swap;
uint32_t g_scratch = 0;
constexpr uint32_t kScratchSize = 1024;

uint32_t ReadLE32(const uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | uint32_t(p[3]) << 24; }
uint16_t ReadLE16(const uint8_t* p) { return uint16_t(p[0] | p[1] << 8); }

}  // namespace

void Apply(rex::memory::Memory* memory, uint32_t image_base, uint32_t image_size) {
  if (!memory || !REXCVAR_GET(translation_patch)) return;
  const auto path = rex::filesystem::GetExecutableFolder() / "translation" / "english.tsv";
  const std::vector<Line> lines = Load(path);
  if (lines.empty()) {
    REXLOG_WARN("[translation] {} missing or empty: the Japanese text stays", path.string());
    return;
  }
  uint8_t* image = memory->TranslateVirtual<uint8_t*>(image_base);

  // The code section, whose words are instructions, not pointers (the image's PE section table).
  uint32_t code_begin = 0, code_end = 0;
  const uint32_t pe = ReadLE32(image + 0x3C);
  const uint16_t sections = ReadLE16(image + pe + 6), optional = ReadLE16(image + pe + 20);
  for (uint16_t i = 0; i < sections; ++i) {
    const uint8_t* section = image + pe + 24 + optional + 40 * i;
    if (std::memcmp(section, ".text", 6) == 0) {
      code_begin = ReadLE32(section + 12);
      code_end = code_begin + ReadLE32(section + 8);
    }
  }

  // The pointers in the image's data, by the address they hold (big-endian guest words).
  std::unordered_map<uint32_t, std::vector<uint32_t>> pointers;
  std::unordered_map<uint32_t, bool> wanted;
  for (const Line& line : lines)
    if (line.in_data) wanted[line.address] = true;
  for (uint32_t offset = 0; offset + 4 <= image_size; offset += 4) {
    if (offset >= code_begin && offset < code_end) continue;
    const uint32_t value =
        uint32_t(image[offset]) << 24 | uint32_t(image[offset + 1]) << 16 | uint32_t(image[offset + 2]) << 8 |
        image[offset + 3];
    if (wanted.count(value)) pointers[value].push_back(offset);
  }

  // The image's read-only data is write-protected; open the whole image while writing.
  memory->LookupHeap(image_base)->Protect(image_base, image_size,
                                           rex::memory::kMemoryProtectRead | rex::memory::kMemoryProtectWrite);
  uint32_t in_place = 0, moved = 0, kept = 0;
  g_scratch = memory->SystemHeapAlloc(kScratchSize);
  for (const Line& line : lines) {
    const uint32_t needed = uint32_t(line.bytes.size()) + 1;
    if (const uint32_t copy = memory->SystemHeapAlloc(needed)) {
      uint8_t* at = memory->TranslateVirtual<uint8_t*>(copy);
      std::memcpy(at, line.bytes.data(), line.bytes.size());
      at[line.bytes.size()] = 0;
      g_swap[line.address] = copy;
    }
    if (needed <= line.capacity) {
      uint8_t* at = memory->TranslateVirtual<uint8_t*>(line.address);
      std::memcpy(at, line.bytes.data(), line.bytes.size());
      std::memset(at + line.bytes.size(), 0, line.capacity - line.bytes.size());
      ++in_place;
    } else if (line.in_data && pointers.count(line.address)) {
      const uint32_t copy = memory->SystemHeapAlloc(needed);
      if (!copy) {
        ++kept;
        continue;
      }
      uint8_t* at = memory->TranslateVirtual<uint8_t*>(copy);
      std::memcpy(at, line.bytes.data(), line.bytes.size());
      at[line.bytes.size()] = 0;
      for (uint32_t offset : pointers[line.address]) {
        image[offset] = uint8_t(copy >> 24);
        image[offset + 1] = uint8_t(copy >> 16);
        image[offset + 2] = uint8_t(copy >> 8);
        image[offset + 3] = uint8_t(copy);
      }
      ++moved;
    } else {
      ++kept;  // reached from code: the hooks swap it for its copy
    }
  }
  REXLOG_INFO("[translation] English translation patch: {} lines in place, {} moved, {} through the text "
              "hooks only", in_place, moved, kept);
}

namespace {

uint32_t Swapped(uint32_t address) {
  const auto it = g_swap.find(address);
  return it == g_swap.end() ? address : it->second;
}

bool IsLead(uint8_t c) { return (c >= 0x81 && c <= 0x9F) || (c >= 0xE0 && c <= 0xEF); }
bool IsLetter(uint8_t c) { return c > 0x20 && c < 0x7F && !(c >= '0' && c <= '9'); }
// In a line with letters digits are drawn with them (the original drawer's are full-width).
bool IsAscii(uint8_t c) { return c > 0x20 && c < 0x7F; }

// Whether the string has a single-byte letter or sign (what the original drawer skips).
bool HasLetters(uint8_t* base, uint32_t text) {
  if (!text) return false;
  for (uint32_t i = 0;; ) {
    const uint8_t c = REX_LOAD_U8(text + i);
    if (!c) return false;
    if (c == 0x0B) { i += 5; continue; }
    if (IsLead(c)) { i += 2; continue; }
    if (IsLetter(c)) return true;
    ++i;
  }
}

// The width of each line of a string with letters, with the drawer's advances.
std::vector<float> LineWidths(uint8_t* base, uint32_t text) {
  std::vector<float> widths{0.0f};
  for (uint32_t i = 0;; ) {
    const uint8_t c = REX_LOAD_U8(text + i);
    if (!c) break;
    if (c == '\n') { widths.push_back(0.0f); ++i; }
    else if (c == 0x0B) i += 5;
    else if (c == 0x09) ++i;
    else if (c == ' ') { widths.back() += 16.0f; ++i; }
    else if (c == 0x07 || c == 0x08) { widths.back() += 33.0f; ++i; }
    else if (IsAscii(c)) { widths.back() += (c == ',' || c == '.') ? 15.0f : 17.0f; ++i; }
    else { widths.back() += 33.0f; i += 2; }
  }
  return widths;
}

// Logs once each line still drawn with Japanese in it (a string the table does not cover), for testing coverage.
void LogJapanese(uint8_t* base, uint32_t text) {
  static std::unordered_map<uint32_t, bool> seen;
  if (!text || g_swap.empty() || seen.count(text)) return;
  seen[text] = true;
  std::string bytes;
  for (uint32_t i = 0; i < 512; ++i) {
    const uint8_t c = REX_LOAD_U8(text + i);
    if (!c) break;
    bytes += char(c);
  }
  for (size_t i = 0; i < bytes.size(); ++i) {
    if (IsLead(uint8_t(bytes[i])) && uint8_t(bytes[i]) != 0x81) {  // kana or kanji (0x81: signs and spaces)
      std::string hex;
      for (uint8_t c : bytes) hex += fmt::format("{:02x}", c);
      REXLOG_INFO("[translation] Japanese line drawn from {:08X}: {}", text, hex);
      return;
    }
  }
}

void StoreFloat(uint8_t* base, uint32_t address, float value) {
  uint32_t bits;
  std::memcpy(&bits, &value, 4);
  REX_STORE_U32(address, bits);
}

}  // namespace
}  // namespace espgaluda2::translation

using namespace espgaluda2::translation;

// The swap is made only where text is measured and drawn: swapped at the dialog set-up (sub_820A0910) the copy's
// address was kept in the dialog and the replay save never finished (its "Saving replay data." dialog stayed up).
// The line-width measure: r4 text, r5 widths out (floats), r6 their count, r7 the widest out; f1 = the height.
extern "C" REX_FUNC(sub_820A15A0) {
  ctx.r4.u64 = Swapped(ctx.r4.u32);
  if (!HasLetters(base, ctx.r4.u32)) {
    __imp__sub_820A15A0(ctx, base);
    return;
  }
  const std::vector<float> widths = LineWidths(base, ctx.r4.u32);
  float widest = 0.0f;
  for (size_t i = 0; i < widths.size(); ++i) {
    widest = std::max(widest, widths[i]);
    if (ctx.r5.u32 && i < ctx.r6.u32) StoreFloat(base, ctx.r5.u32 + 4 * uint32_t(i), widths[i]);
  }
  if (ctx.r7.u32) StoreFloat(base, ctx.r7.u32, widest);
  ctx.f1.f64 = 33.0 * double(widths.size());
}

// The text drawer: f1/f2 position, f3 box width, f4, r6 text, r8 line widths (or 0), r9 alignment flags, r10 layer;
// stack: [r1+84] final x out, [r1+92] "ended with a line break" out; f1 = the final y.
extern "C" REX_FUNC(sub_820A16A8) {
  ctx.r6.u64 = Swapped(ctx.r6.u32);
  LogJapanese(base, ctx.r6.u32);
  if (!g_scratch || !HasLetters(base, ctx.r6.u32)) {
    __imp__sub_820A16A8(ctx, base);
    return;
  }
  const uint32_t text = ctx.r6.u32, flags = ctx.r9.u32, layer = ctx.r10.u32;
  const bool have_widths = ctx.r8.u32 != 0;
  const double x0 = ctx.f1.f64, box = ctx.f3.f64, f4 = ctx.f4.f64;
  double y = ctx.f2.f64;
  const uint32_t out_x = REX_LOAD_U32(ctx.r1.u32 + 84), out_break = REX_LOAD_U32(ctx.r1.u32 + 92);
  const std::vector<float> widths = LineWidths(base, text);
  size_t line = 0;
  // The drawer's line starts: left (flags 0 or bit 0) at x, and the lines after a break at x + f4; centred (bit 1) or
  // right-aligned (bit 2) by the line's width when the caller passed widths, else at x.
  double line_start = x0;
  auto line_x = [&]() {
    const double w = widths[std::min(line, widths.size() - 1)];
    if (flags == 0 || (flags & 1)) return line > 0 ? x0 + f4 : x0;
    if (flags & 2) return have_widths ? x0 + box * 0.5 - w * 0.5 : x0;
    if (flags & 4) return have_widths ? x0 + box - w : x0;
    return line_start;
  };
  double x = line_start = line_x();
  uint32_t colour = 0xFF808080u;
  std::vector<uint8_t> run;
  bool run_letters = false;
  double run_x = x;
  auto flush = [&]() {
    if (run.empty()) return;
    std::vector<uint8_t> bytes;
    if (!run_letters) {  // the colour for the original drawer, as its own code
      bytes = {0x0B, uint8_t(colour >> 24), uint8_t(colour >> 16), uint8_t(colour >> 8), uint8_t(colour)};
    }
    bytes.insert(bytes.end(), run.begin(), run.end());
    bytes.resize(std::min<size_t>(bytes.size(), kScratchSize - 1));
    for (size_t i = 0; i < bytes.size(); ++i) REX_STORE_U8(g_scratch + uint32_t(i), bytes[i]);
    REX_STORE_U8(g_scratch + uint32_t(bytes.size()), 0);
    const PPCRegister r1 = ctx.r1;
    ctx.f1.f64 = run_x;
    ctx.f2.f64 = y;
    ctx.r6.u64 = g_scratch;
    if (run_letters) {
      ctx.r7.u64 = colour;
      ctx.r8.u64 = layer;
      sub_820A27A0(ctx, base);
    } else {
      ctx.f3.f64 = box;
      ctx.f4.f64 = f4;
      ctx.r8.u64 = 0;
      ctx.r9.u64 = 1;
      ctx.r10.u64 = layer;
      __imp__sub_820A16A8(ctx, base);
    }
    ctx.r1 = r1;
    run.clear();
  };
  auto add = [&](bool letters, std::initializer_list<uint8_t> bytes, double advance) {
    if (!run.empty() && letters != run_letters) flush();
    if (run.empty()) {
      run_letters = letters;
      run_x = x;
    }
    run.insert(run.end(), bytes);
    x += advance;
  };
  uint8_t last = 0;
  for (uint32_t i = 0;; ) {
    const uint8_t c = REX_LOAD_U8(text + i);
    if (!c) break;
    last = c;
    if (c == '\n') {
      flush();
      y += 33.0;
      ++line;
      x = line_start = line_x();
      ++i;
    } else if (c == 0x0B) {
      flush();
      colour = REX_LOAD_U8(text + i + 1) << 24 | REX_LOAD_U8(text + i + 2) << 16 | REX_LOAD_U8(text + i + 3) << 8 |
               REX_LOAD_U8(text + i + 4);
      i += 5;
    } else if (c == 0x09) {
      ++i;
    } else if (c == ' ') {
      flush();
      x += 16.0;
      ++i;
    } else if (c == 0x07 || c == 0x08) {
      add(false, {c}, 33.0);
      ++i;
    } else if (IsAscii(c)) {
      add(true, {c}, (c == ',' || c == '.') ? 15.0 : 17.0);
      ++i;
    } else {
      add(false, {c, REX_LOAD_U8(text + i + 1)}, 33.0);
      i += 2;
    }
  }
  flush();
  if (out_x) StoreFloat(base, out_x, float(x));
  if (out_break) REX_STORE_U32(out_break, last == '\n' ? 1u : 0u);
  ctx.f1.f64 = y;
}

namespace espgaluda2::translation {

// The tutorial's controller diagram (the Black Label, Xbox 360 and Arrange lessons) labels its buttons with Japanese
// art: four sprites in the game's 1024x1024 interface texture, at the rectangles the game's sprite table holds
// (default.xex data 0x82380C0C..: x 16 / 80 / 144 / 208, y 432, 48 wide, 16 or 32 high). With the patch on, the
// renderer hands each 4-byte texture it uploads to EnglishTextureLabels (renut textures.cpp); the interface texture
// (identified by the XXH3 of its level 0) gets those rectangles redrawn with the official Switch English button names
// (XBOX_CCFG_TEXT_LSHOT / RSHOT / LOCKSHOT / BOMB: shot, Ascension, guard barrier, rapid-fire shot), in the game's
// own ASCII font (the 16x32 cells of xbox_sys_01, kept from that texture's own upload) at half size, area-averaged,
// narrowed to the 48 pixels a line has where needed. Bytes per texel: three colour bytes (each treated alike), then
// alpha.
namespace {

constexpr uint64_t kFontTextureHash = 0x53F7F18595878C11ull;  // xbox_sys_01 (512x512, the ASCII font)
constexpr uint64_t kLabelTextureHash = 0xA3D9A45A9EE82AA8ull;  // the interface texture with the diagram labels
std::vector<uint8_t> g_font;                                     // xbox_sys_01's texels, 512x512x4

struct Texel {
  float a = 0, c[3] = {0, 0, 0};  // straight colour
};

// One glyph of the 16x32 font, area-averaged (premultiplied) to width x 16.
std::vector<Texel> Glyph(char ch, int width) {
  std::vector<Texel> out(size_t(width) * 16);
  const int index = ch - 32, u = (index * 16) % 512, v = ((index * 16) / 512 + 2) * 32;
  const float sx = 16.0f / float(width);
  for (int oy = 0; oy < 16; ++oy) {
    for (int ox = 0; ox < width; ++ox) {
      float sum_a = 0, sum_c[3] = {0, 0, 0}, area = 0;
      const float x0 = ox * sx, x1 = (ox + 1) * sx;
      for (int sy = oy * 2; sy < oy * 2 + 2; ++sy) {
        for (int x = int(x0); x < int(std::ceil(x1)); ++x) {
          const float w = std::min(x1, float(x + 1)) - std::max(x0, float(x));
          if (w <= 0) continue;
          const uint8_t* t = &g_font[(size_t(v + sy) * 512 + u + x) * 4];
          const float a = t[3] / 255.0f;
          sum_a += a * w;
          for (int k = 0; k < 3; ++k) sum_c[k] += t[k] / 255.0f * a * w;
          area += w;
        }
      }
      Texel& o = out[size_t(oy) * width + ox];
      o.a = sum_a / area;
      for (int k = 0; k < 3; ++k) o.c[k] = sum_a > 0 ? sum_c[k] / sum_a : 0;
    }
  }
  return out;
}

// A line of text at half the font's size (advance 8.5, 7.5 for ',' and '.', 8 a space) scaled by 'scale' in x,
// cropped to its ink.
std::vector<Texel> Line(const std::string& text, float scale, int& width) {
  auto advance = [](char c) { return c == ' ' ? 8.0f : (c == ',' || c == '.') ? 7.5f : 8.5f; };
  float total = 0;
  for (char c : text) total += advance(c);
  const int canvas = int(std::ceil(total * scale)) + 10;
  std::vector<Texel> out(size_t(canvas) * 16);
  float x = 0;
  for (char c : text) {
    if (c != ' ') {
      const int gw = std::max(1, int(std::lround(8 * scale + 0.4f)));
      const std::vector<Texel> g = Glyph(c, gw);
      const int xi = int(std::lround(x));
      for (int y = 0; y < 16; ++y) {
        for (int gx = 0; gx < gw && xi + gx < canvas; ++gx) {
          const Texel& s = g[size_t(y) * gw + gx];
          Texel& d = out[size_t(y) * canvas + xi + gx];
          const float a = s.a + d.a * (1 - s.a);
          for (int k = 0; k < 3; ++k) d.c[k] = a > 0 ? (s.c[k] * s.a + d.c[k] * d.a * (1 - s.a)) / a : 0;
          d.a = a;
        }
      }
    }
    x += advance(c) * scale;
  }
  int first = canvas, last = -1;
  for (int cx = 0; cx < canvas; ++cx)
    for (int y = 0; y < 16; ++y)
      if (out[size_t(y) * canvas + cx].a > 0.02f) first = std::min(first, cx), last = std::max(last, cx);
  if (last < first) {
    width = 0;
    return {};
  }
  width = last - first + 1;
  std::vector<Texel> cropped(size_t(width) * 16);
  for (int y = 0; y < 16; ++y)
    for (int cx = 0; cx < width; ++cx) cropped[size_t(y) * width + cx] = out[size_t(y) * canvas + first + cx];
  return cropped;
}

}  // namespace

bool TextureLabelsEnabled() { return REXCVAR_GET(translation_patch); }

void EnglishTextureLabels(uint64_t hash, uint8_t* texels, uint32_t width, uint32_t height, uint32_t row_pitch) {
  if (!REXCVAR_GET(translation_patch)) return;
  if (hash == kFontTextureHash && width == 512 && height == 512) {
    g_font.resize(size_t(512) * 512 * 4);
    for (uint32_t y = 0; y < 512; ++y) std::memcpy(&g_font[size_t(y) * 512 * 4], texels + size_t(y) * row_pitch, 512 * 4);
    return;
  }
  if (hash != kLabelTextureHash || width != 1024 || height != 1024) return;
  if (g_font.empty()) {
    REXLOG_WARN("[translation] tutorial diagram texture before the font texture: its labels stay Japanese");
    return;
  }
  struct Label {
    int x;
    std::vector<std::string> lines;
  };
  const Label labels[] = {{16, {"Shot"}}, {80, {"Ascension"}}, {144, {"Guard", "Barrier"}}, {208, {"Rapid-fire", "Shot"}}};
  constexpr int kTop = 432, kWidth = 48;
  for (const Label& label : labels) {
    for (int y = kTop; y < kTop + 16 * int(label.lines.size()); ++y)
      std::memset(texels + size_t(y) * row_pitch + size_t(label.x) * 4, 0, kWidth * 4);
    for (size_t k = 0; k < label.lines.size(); ++k) {
      int w = 0;
      std::vector<Texel> line = Line(label.lines[k], 1.0f, w);
      if (w > kWidth - 2) line = Line(label.lines[k], float(kWidth - 2) / float(w), w);
      const int x0 = label.x + (kWidth - w) / 2, y0 = kTop + 16 * int(k);
      for (int y = 0; y < 16; ++y) {
        for (int x = 0; x < w; ++x) {
          const Texel& s = line[size_t(y) * w + x];
          uint8_t* d = texels + size_t(y0 + y) * row_pitch + size_t(x0 + x) * 4;
          for (int c = 0; c < 3; ++c) d[c] = uint8_t(std::lround(std::clamp(s.c[c], 0.0f, 1.0f) * 255));
          d[3] = uint8_t(std::lround(std::clamp(s.a, 0.0f, 1.0f) * 255));
        }
      }
    }
  }
  REXLOG_INFO("[translation] tutorial controller diagram labels drawn in English");
}

}  // namespace espgaluda2::translation
