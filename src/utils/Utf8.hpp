#pragma once

#include <string>

namespace timetracker::utils {

/**
 * UTF-8 text helpers. The app is UTF-8 internally, so anything that has to look at characters
 * rather than bytes goes through here.
 */
class Utf8 {
public:
    /**
     * Lowercase a UTF-8 string, locale-independently, for every script that has case
     * (Latin, Cyrillic, Greek, Armenian, Georgian, Deseret, Adlam, …). Caseless scripts pass
     * through unchanged, which is what lowercasing them means.
     *
     * Backed by the generated [UnicodeLowercase.hpp] table, which replaced ICU: it was linked
     * for this one operation and carried ~30 MB of Unicode data into the binary.
     *
     * Bytes that are not valid UTF-8 are copied through untouched, so a malformed name still
     * yields a stable search key instead of a replacement character.
     *
     * @param text UTF-8 encoded text
     * @return Lowercased UTF-8 text
     */
    static std::string toLower(const std::string& text);
};

}  // namespace timetracker::utils
