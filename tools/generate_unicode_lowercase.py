#!/usr/bin/env python3
"""Generate src/utils/UnicodeLowercase.hpp from the Unicode data built into Python.

The app needs locale-independent UTF-8 lowercasing for activity search names and used to link
ICU for it, which cost ~30 MB of statically linked Unicode data for this single operation. The
mappings are a table instead: every code point whose lowercase form differs from itself, taken
from Python's own Unicode database, which matches what ICU's toLower() produced.

Run after a Python upgrade to pick up a newer Unicode version:

    python tools/generate_unicode_lowercase.py

The written header is checked in; the build does not invoke this script.
"""

import sys
import unicodedata
from pathlib import Path

HEADER = Path(__file__).resolve().parent.parent / "src" / "utils" / "UnicodeLowercase.hpp"

# Widest lowercase expansion the generated table can hold; the C++ side reads exactly two slots.
MAX_EXPANSION = 2


def collect():
    mappings = []
    for cp in range(0x110000):
        ch = chr(cp)
        lower = ch.lower()
        if lower == ch:
            continue
        codes = [ord(c) for c in lower]
        if len(codes) > MAX_EXPANSION:
            raise SystemExit(
                f"U+{cp:04X} lowercases to {len(codes)} code points, "
                f"but the table holds at most {MAX_EXPANSION}"
            )
        while len(codes) < MAX_EXPANSION:
            codes.append(0)
        mappings.append((cp, codes))
    return mappings


def main():
    mappings = collect()

    lines = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "",
        "// GENERATED FILE - do not edit by hand.",
        "// Regenerate with: python tools/generate_unicode_lowercase.py",
        f"// Unicode {unicodedata.unidata_version} (Python {sys.version_info.major}."
        f"{sys.version_info.minor} unicodedata)",
        "//",
        "// Every code point whose lowercase form differs from itself, sorted by code point so",
        "// Utf8::toLower can binary search it. A mapping expands to at most two code points",
        "// (U+0130 LATIN CAPITAL LETTER I WITH DOT ABOVE is the notable one); `second` is 0 when",
        "// the lowercase form is a single code point.",
        "",
        "namespace timetracker::utils::unicode_data {",
        "",
        "struct LowercaseMapping {",
        "    char32_t upper;",
        "    char32_t first;",
        "    char32_t second;",
        "};",
        "",
        "inline constexpr LowercaseMapping kLowercaseMappings[] = {",
    ]

    for cp, codes in mappings:
        name = unicodedata.name(chr(cp), "")
        comment = f"  // {name}" if name else ""
        lines.append(
            f"    {{0x{cp:04X}, 0x{codes[0]:04X}, 0x{codes[1]:04X}}},{comment}"
        )

    lines += [
        "};",
        "",
        "inline constexpr std::size_t kLowercaseMappingCount =",
        "    sizeof(kLowercaseMappings) / sizeof(kLowercaseMappings[0]);",
        "",
        "}  // namespace timetracker::utils::unicode_data",
        "",
    ]

    HEADER.write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print(f"{HEADER}: {len(mappings)} mappings, Unicode {unicodedata.unidata_version}")


if __name__ == "__main__":
    main()
