#!/usr/bin/env python3
"""espgaluda2-recomp (2026-10-30, original tool): the English text of the official Switch release for the Xbox 360
game's Japanese strings.

Finds every Japanese string in the Xbox 360 default.xex image (tools/xex/xex_image.py) and matches it, ignoring line
breaks and colour and button codes, against the Japanese text table of the Switch release (res/DISKDATA/text/
out_nx_jp.btxt). The English line with the same ID (out_nx_en.btxt) is converted to the Xbox 360 game's bytes:
Shift-JIS, \n a line break, \a / \b with their _STR_KE_STR_W the A / B button glyphs (0x07 / 0x08), and the
Switch's colour tokens the Xbox 360 game's colour codes (0x0B 0xFF R G B; the pairs were found by aligning the 192
coloured strings both releases share, all consistent). Output, one line per string: guest address, the bytes it may
take in place (the string, its terminator and the zero padding after it), whether its address is in the image's data
(a pointer table the port can redirect, 1) or only in code (0), the Switch ID and the English bytes in hex.
src/translation.cpp writes them at load.

  python3 -I tools/translation/make_map.py <default_image.bin> <out_nx_jp.btxt> <out_nx_en.btxt> <out.tsv>
"""
import re
import struct
import sys

COLOURS = {"_STR_D": "505050", "_STR_DP": "801040", "_STR_G": "108010", "_STR_LB": "104080", "_STR_O": "804010",
           "_STR_P": "801080", "_STR_R": "801010", "_STR_W": "808080", "_STR_Y": "808010"}


# The Switch's tokens are not delimited ("_STR_DYou"): matched from this list, longest first.
TOKENS = r'_STR_(?:DP|KE|LB|D|G|O|P|R|W|Y)'


def to_xbox(text):
    # _STR_KE ... _STR_W: the Switch's keyboard-key hint after a button glyph, which the Xbox 360 game has not (its
    # "\x07はい"): the glyph stays, the hint goes. A _STR_KE elsewhere colours nothing on the Xbox 360 either.
    text = re.sub(r'\\([ab])_STR_KE[^_]*_STR_W', lambda m: "\x07" if m.group(1) == "a" else "\x08", text)
    text = text.replace("_STR_KE", "").replace("\\n", "\n").replace("\\a", "\x07").replace("\\b", "\x08")
    # The full-width space between the Japanese choices ("はい\u3000\u3000"); English spaces are ASCII (16 px each).
    text = text.replace("\u3000", "  ")
    out = b""
    for part in re.split('(' + TOKENS + ')', text):
        if part in COLOURS:
            out += b"\x0b\xff" + bytes.fromhex(COLOURS[part])
        elif part.startswith("_STR_"):
            raise ValueError("unknown token " + part)
        else:
            out += part.encode("cp932")
    return out

IMAGE_BASE = 0x82000000
CORRECTIONS = __file__.rsplit("/", 1)[0] + "/corrections.tsv"
OVERRIDES = __file__.rsplit("/", 1)[0] + "/overrides.tsv"
JAPANESE = re.compile(r'[぀-ヿ一-龯Ａ-ｚ]')

# A whole string of the drawer's bytes: ASCII, line break, tab, button glyphs, colour codes, Shift-JIS pairs.
CLEAN = re.compile(rb'(?:[\x20-\x7e\n\x07\x08\x09]|\x0b\xff...|[\x81-\x9f\xe0-\xef][\x40-\xfc])*\Z', re.S)


def table(path, encoding):
    # One entry per CRLF-ended line; a bare LF inside an entry only wraps the file's text (its line breaks are the
    # literal \n escapes) and is dropped.
    entries = {}
    for line in open(path, "rb").read().decode(encoding).split("\r\n"):
        if "," in line:
            key, value = line.split(",", 1)
            entries[key] = value.replace("\n", "")
    return entries


def readable(text):
    # A Switch line as readable text: line breaks as " / ", without colour and button codes.
    text = re.sub(r'\\[ab]?' + TOKENS + '|' + TOKENS, '', text).replace("\\a", "(A) ").replace("\\b", "(B) ")
    return re.sub(r'\s*\\n\s*', ' / ', text).strip(" /")


def norm(text):
    text = re.sub(r'\\[ab]?' + TOKENS + '|' + TOKENS + r'|\\n|\\[ab]', '', text)
    text = re.sub(r'[\x00-\x1f]', '', text)
    return re.sub(r'[\s■　]+', '', text)


def main():
    image_path, jp_path, en_path, out_path = sys.argv[1:5]
    image = open(image_path, "rb").read()
    jp = table(jp_path, "cp932")
    en = table(en_path, "cp932")
    # tools/translation/corrections.tsv: grammar and spelling fixes to official lines, by ID.
    for line in open(CORRECTIONS, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        key, old, new = line.rstrip("\n").split("\t")
        if old not in en[key]:
            raise ValueError(f"correction for {key} does not apply: {old}")
        en[key] = en[key].replace(old, new)
    changes = ["Corrected lines (the official Switch line, then the line shown):", ""]
    for line in open(CORRECTIONS, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        key, old, new = line.rstrip("\n").split("\t")
        changes += [f"{key}", f"  was: {readable(old)}", f"  now: {readable(new)}", ""]
    # Some entries of the English table are untranslated Japanese: an ID with English wins among IDs with the same
    # Japanese text, and a string with only Japanese ones is left as it is.
    index = {}
    for key, value in jp.items():
        if key in en and not JAPANESE.search(en[key]):
            index.setdefault(norm(value), key)
    # tools/translation/overrides.tsv: address, source ID, English (empty: the source ID's official line).
    overrides, adapted = {}, []
    for line in open(OVERRIDES, encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        address, source, *text = line.rstrip("\n").split("\t")
        overrides[int(address, 16)] = (source, text[0] if text and text[0] else en[source])
        if source.startswith("adapted:"):
            adapted.append(f"{source[8:]}\n  now: {readable(overrides[int(address, 16)][1])}\n")
    # The game's text is in the image's read-only data section (its PE section table).
    pe = struct.unpack_from("<I", image, 0x3C)[0]
    sections, optional = struct.unpack_from("<H", image, pe + 6)[0], struct.unpack_from("<H", image, pe + 20)[0]
    for i in range(sections):
        section = pe + 24 + optional + 40 * i
        if image[section:section + 8].rstrip(b"\0") == b".rdata":
            size, begin = struct.unpack_from("<II", image, section + 8)
    rows, strings = [], 0
    for m in re.compile(rb'[^\x00]{4,}\x00').finditer(image, begin, begin + size):
        # A string can follow other data with no zero between (a float before XTEXT_Arrange_22_03's line): it starts
        # at the first 4-byte aligned offset from which every byte is text (the drawer's codes, ASCII, Shift-JIS).
        start = next((k for k in range(m.start(), m.end() - 1) if k % 4 == 0 and CLEAN.match(image, k, m.end() - 1)),
                     None)
        if start is None:
            continue
        raw = image[start:m.end() - 1]
        plain = re.sub(rb'\x0b\xff...', b'', raw, flags=re.S)
        try:
            text = plain.decode("cp932")
        except UnicodeDecodeError:
            continue
        if len(JAPANESE.findall(text)) < 2:
            continue
        strings += 1
        address = IMAGE_BASE + start
        if address in overrides:
            key, english = overrides[address]
        elif norm(text) in index:
            key = index[norm(text)]
            english = en[key]
        else:
            continue
        end = m.end()
        while end < len(image) and image[end] == 0:
            end += 1
        in_data = 1 if struct.pack(">I", address) in image else 0
        rows.append((address, end - start, in_data, key, to_xbox(english)))
    changes += ["Adapted lines (Xbox 360 messages the Switch has not, made from these official Switch lines):", ""]
    changes_tail = ["Tutorial controller diagram labels (Japanese art, redrawn in the game's own English letters; the",
                    "official Switch names of these buttons, from XBOX_CCFG_TEXT_LSHOT / RSHOT / LOCKSHOT / BOMB):", "",
                    "  ショット -> Shot", "  覚聖 -> Ascension", "  ガードバリア -> Guard Barrier",
                    "  ショット連射 -> Rapid-fire Shot", ""]
    with open(out_path.rsplit("/", 1)[0] + "/CHANGES.txt", "w", encoding="utf-8") as out:
        out.write("\n".join(changes + adapted + [""] + changes_tail))
    fit = sum(1 for r in rows if len(r[4]) + 1 <= r[1])
    moved = sum(1 for r in rows if len(r[4]) + 1 > r[1] and r[2])
    with open(out_path, "w") as out:
        out.write("# address\tcapacity\tin_data\tswitch_id\tenglish (hex)  (tools/translation/make_map.py)\n")
        for address, capacity, in_data, key, data in rows:
            out.write(f"{address:08X}\t{capacity}\t{in_data}\t{key}\t{data.hex()}\n")
    print(f"{strings} Japanese strings, {len(rows)} with an official English line: {fit} fit in place, {moved} move "
          f"through a pointer table, {len(rows) - fit - moved} too long and reached from code")


if __name__ == "__main__":
    main()
