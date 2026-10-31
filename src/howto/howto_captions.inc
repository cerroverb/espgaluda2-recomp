// espgaluda2-recomp (2026-10-31, original code): the English translation patch for the "how to play" captions shown
// after character select, before the first stage, in every mode. Included by one howto_datNN.cpp per mode module,
// which defines the module's addresses (found in its generated code):
//   HOWTO_DRAWER  the caption drawer (draws the 4 x 13 grid of glyph sprites)    HOWTO_SPRITE  its sprite call
//   HOWTO_STATE   the caption state (+4: 52 u32 grid cells = glyph cell + 412; +214: u16 script index)
//   HOWTO_SCRIPT  the caption script (u16 big-endian ops; text op: 0, delay, count, speed, column, line, glyphs...)
// The game types each caption into the grid one glyph a step, 16 px a glyph. With the patch on, the drawer hook finds
// which caption the grid holds (its glyphs against the script's), and draws that caption's English instead, typed in
// step with the Japanese (the same share of its letters), with the caption font's own Latin letters at their own
// widths (16 px cells, ink measured from the font sheet), word-wrapped to the box (x 30..238, 4 lines). The script,
// its timing and the art are unchanged; with the patch off the original drawer runs.
#include "espgaluda2_recomp_init.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <rex/cvar.h>

extern "C" REX_FUNC(HOWTO_IMP(HOWTO_DRAWER_NAME));

namespace {

// The 20 captions' English (owner-approved wording, 2026-10-31; official Switch tutorial lines where one matches).
const char* const kEnglish[] = {
    "Short press the shot button to fire a shot.",
    "Hold the shot button to fire a rapier.",
    "Your movement speed is slower while the rapier is firing.",
    "The Ascension button turns Ascension on and off.",
    "When you ascend, your movement speed changes,",
    "Ageha and Asagi get a stronger rapier, Tateha stronger shots.",
    "If you ascend while you have ascension counter,",
    "you deploy the Ascension Dead Zone, which slows the whole screen.",
    "Destroying an enemy in the Ascension Dead Zone cancels its bullets.",
    "If your ascension counter runs out during Ascension,",
    "enemy bullets speed up, so be careful.",
    "During Ascension, if you are hit with barrier gauge left, the barrier activates.",
    "When you have ascension counter and gold counter,",
    "holding the Ascension button deploys the Ascension Absolute Dead Zone.",
    "Destroying an enemy in it cancels its bullets at first,",
    "but its bullets and its body fire return shots.",
    "Destroying enemies cancels return shots, but they fire more return shots.",
    "Use this well and aim for an even higher score!",
    "Hold the guard barrier button to raise a guard barrier.",
    "Release it to fire a guard barrier attack as strong as you held it.",
};
constexpr int kCaptions = int(sizeof(kEnglish) / sizeof(kEnglish[0]));

#ifdef HOWTO_HD
constexpr bool kHd = true;
#else
constexpr bool kHd = false;
#endif

struct Glyph {
  int cell = -1;  // caption font cell (glyph sprite = cell + 412)
  int ink = 0;    // first ink column in the 16 px cell
  int width = 0;  // advance
};

// The caption font sheet: 'A'..'Z' cells 1..26, 'a'..'n' 34..47, 'o'..'z' 48..59; ',' 257, '.' 258, '!' 262. Ink columns
// measured from the sheet (in the mode's texture atlas).
Glyph Lookup(char c) {
  static const uint8_t upper[26][2] = {{1, 11}, {1, 11}, {1, 11}, {0, 11}, {1, 11}, {1, 11}, {1, 12}, {0, 12}, {4, 8},
                                       {2, 9},  {0, 12}, {1, 11}, {0, 12}, {0, 12}, {0, 11}, {1, 11}, {0, 12}, {1, 11},
                                       {2, 10}, {1, 11}, {0, 12}, {0, 12}, {0, 13}, {1, 11}, {1, 11}, {1, 11}};
  static const uint8_t lower[26][2] = {{2, 11}, {3, 12}, {2, 10}, {0, 10}, {1, 9}, {4, 11}, {1, 11}, {2, 13}, {4, 8},
                                       {1, 7},  {1, 11}, {4, 8},  {0, 11}, {1, 12}, {1, 10}, {2, 12}, {1, 11}, {3, 12},
                                       {2, 9},  {3, 10}, {1, 11}, {1, 10}, {0, 11}, {1, 11}, {1, 12}, {2, 12}};
  // The Xbox 360 modes' sheet (HOWTO_HD: dat12, dat163) draws the same cells with other letter shapes; their ink was
  // measured on screen (every letter in its own cell), in the same 16-unit cell.
  static const int8_t upper_hd[26][2] = {{3, 14}, {3, 13}, {2, 13}, {2, 13}, {3, 13}, {4, 13}, {3, 14}, {2, 14}, {6, 10},
                                         {4, 10}, {3, 14}, {4, 13}, {1, 15}, {2, 14}, {2, 14}, {3, 13}, {2, 14}, {3, 14},
                                         {3, 13}, {2, 13}, {3, 13}, {2, 14}, {0, 16}, {0, 14}, {2, 14}, {3, 13}};
  static const int8_t lower_hd[26][2] = {{1, 14}, {1, 15}, {2, 14}, {1, 15}, {1, 14}, {4, 13}, {1, 15}, {1, 15}, {5, 10},
                                         {2, 10}, {1, 15}, {5, 10}, {0, 16}, {0, 15}, {1, 15}, {0, 15}, {1, 16}, {3, 14},
                                         {2, 14}, {3, 12}, {1, 15}, {1, 15}, {0, 15}, {1, 14}, {1, 15}, {1, 14}};
  Glyph g;
  if (c >= 'A' && c <= 'Z') {
    g.cell = 1 + (c - 'A');
    g.ink = kHd ? upper_hd[c - 'A'][0] : upper[c - 'A'][0];
    g.width = (kHd ? upper_hd[c - 'A'][1] : upper[c - 'A'][1]) - g.ink + (kHd ? 1 : 2);
  } else if (c >= 'a' && c <= 'z') {
    g.cell = c <= 'n' ? 34 + (c - 'a') : 48 + (c - 'o');
    g.ink = kHd ? lower_hd[c - 'a'][0] : lower[c - 'a'][0];
    g.width = (kHd ? lower_hd[c - 'a'][1] : lower[c - 'a'][1]) - g.ink + (kHd ? 1 : 2);
  } else if (c == ',') {
    g = {257, 5, 5};
  } else if (c == '.') {
    g = {258, 5, 5};
  } else if (c == '!') {
    g = {262, 5, 5};
  } else {
    g.width = 5;  // space
  }
  return g;
}

// Letters may sit closer (tighten: units off each letter's advance) when a caption would not fit 4 lines.
int Advance(char c, int tighten) {
  const Glyph g = Lookup(c);
  return g.cell >= 0 && c != '.' && c != ',' && c != '!' ? g.width - tighten : g.width;
}

int Width(const std::string& s, int tighten) {
  int w = 0;
  for (char c : s) w += Advance(c, tighten);
  return w;
}

std::vector<std::string> Wrap(const char* text, int tighten) {
  constexpr int kBoxWidth = 208;
  std::vector<std::string> lines(1);
  std::string word;
  auto place = [&]() {
    if (word.empty()) return;
    const std::string joined = lines.back().empty() ? word : lines.back() + " " + word;
    if (Width(joined, tighten) <= kBoxWidth || lines.back().empty())
      lines.back() = joined;
    else
      lines.push_back(word);
    word.clear();
  };
  for (const char* p = text; *p; ++p) {
    if (*p == ' ')
      place();
    else
      word += *p;
  }
  place();
  return lines;
}

struct Caption {
  uint32_t start = 0;                      // script index of its first text op
  std::vector<std::vector<uint16_t>> jp;   // per line: the Japanese glyph cells
  int total = 0;                           // Japanese glyph count
  std::vector<std::string> en;             // English lines
  int tighten = 0;                         // see Advance
  int en_letters = 0;
};

std::vector<Caption> g_captions;
bool g_parsed = false;

void Parse(uint8_t* base) {
  g_parsed = true;
  uint32_t i = 0;
  while (int(g_captions.size()) <= kCaptions && i < 0x2000) {
    const uint32_t a = HOWTO_SCRIPT + i * 2;
    const uint16_t op = REX_LOAD_U16(a), n = REX_LOAD_U16(a + 4), line = REX_LOAD_U16(a + 10);
    bool text = op == 0 && n >= 1 && n <= 13 && line <= 3;
    for (uint32_t k = 0; text && k < n; ++k) text = REX_LOAD_U16(a + 12 + 2 * k) < 344;
    if (!text) {
      ++i;
      continue;
    }
    if (line == 0) {
      if (int(g_captions.size()) == kCaptions) break;
      g_captions.emplace_back();
      g_captions.back().start = i;
    }
    if (!g_captions.empty()) {
      Caption& c = g_captions.back();
      c.jp.resize(line + 1);
      for (uint32_t k = 0; k < n; ++k) c.jp[line].push_back(REX_LOAD_U16(a + 12 + 2 * k));
      c.total += n;
    }
    i += 6 + n;
  }
  if (int(g_captions.size()) != kCaptions) {
    g_captions.clear();  // not the script this hook knows: leave the captions as they are
    return;
  }
  for (int k = 0; k < kCaptions; ++k) {
    for (int t = 0; t <= 3; ++t) {  // the game's letter spacing, or a little closer if the caption needs it
      g_captions[k].en = Wrap(kEnglish[k], t);
      g_captions[k].tighten = t;
      if (g_captions[k].en.size() <= 4) break;
    }
    for (const auto& l : g_captions[k].en) g_captions[k].en_letters += int(l.size());
  }
}

bool Enabled() {
  static const bool enabled = REXCVAR_QUERY(bool, translation_patch);
  return enabled;
}

}  // namespace

extern "C" REX_FUNC(HOWTO_DRAWER_NAME) {
  if (!Enabled()) {
    HOWTO_IMP(HOWTO_DRAWER_NAME)(ctx, base);
    return;
  }
  if (!g_parsed) Parse(base);
  // The grid as the game typed it.
  uint16_t grid[4][13];
  int typed = 0;
  for (int l = 0; l < 4; ++l)
    for (int c = 0; c < 13; ++c) {
      const uint32_t v = REX_LOAD_U32(HOWTO_STATE + 4 + 4 * (l * 13 + c));
      grid[l][c] = v ? uint16_t(v - 412) : 0xFFFF;
      typed += v ? 1 : 0;
    }
  if (typed == 0) return;
  // The caption it holds: the latest (by script position) whose glyphs agree with every typed cell.
  const uint32_t index = REX_LOAD_U16(HOWTO_STATE + 214);
  int found = -1;
  for (int k = 0; k < int(g_captions.size()); ++k) {
    const Caption& cap = g_captions[k];
    bool match = true;
    for (int l = 0; l < 4 && match; ++l)
      for (int c = 0; c < 13 && match; ++c) {
        if (grid[l][c] == 0xFFFF) continue;
        match = l < int(cap.jp.size()) && c < int(cap.jp[l].size()) && cap.jp[l][c] == grid[l][c];
      }
    if (match && (found < 0 || cap.start <= index)) found = k;
  }
  if (found < 0) {  // not a known caption: the game's own drawing
    HOWTO_IMP(HOWTO_DRAWER_NAME)(ctx, base);
    return;
  }
  const Caption& cap = g_captions[found];
  int show = typed >= cap.total ? cap.en_letters : (cap.en_letters * typed + cap.total - 1) / cap.total;
  const PPCRegister r1 = ctx.r1;
  const uint64_t lr = ctx.lr;
  for (int l = 0; l < int(cap.en.size()) && l < 4 && show > 0; ++l) {
    int x = 30;
    const int y = 74 - 16 * l;
    for (char ch : cap.en[l]) {
      if (show-- <= 0) break;
      const Glyph g = Lookup(ch);
      if (g.cell >= 0) {
        ctx.r3.u64 = uint32_t(g.cell + 412);
        ctx.r4.u64 = uint32_t((x - g.ink - 6) << 16);
        ctx.r5.u64 = uint32_t(y << 16);
        ctx.r6.u64 = 238;
        HOWTO_SPRITE(ctx, base);
        ctx.r1 = r1;
      }
      x += Advance(ch, cap.tighten);
    }
  }
  ctx.lr = lr;
}
