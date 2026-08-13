#include "utils/Utf8.hpp"
#include "utils/UnicodeLowercase.hpp"

#include <algorithm>
#include <cstdint>

namespace timetracker::utils {

namespace {

using unicode_data::kLowercaseMappingCount;
using unicode_data::kLowercaseMappings;
using unicode_data::LowercaseMapping;

// How many bytes the sequence starting with this lead byte occupies, or 0 if it cannot start one.
int sequenceLength(unsigned char lead) {
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 0;
}

// Decode the sequence at `index`, advancing it past what was consumed. Returns false and leaves
// `index` on the offending byte when the sequence is malformed or over-long.
bool decode(const std::string& text, std::size_t& index, char32_t& codePoint) {
    const auto lead = static_cast<unsigned char>(text[index]);
    const int length = sequenceLength(lead);
    if (length == 0 || index + static_cast<std::size_t>(length) > text.size()) {
        return false;
    }

    static constexpr char32_t kLeadMask[] = {0, 0x7F, 0x1F, 0x0F, 0x07};
    char32_t value = lead & kLeadMask[length];

    for (int i = 1; i < length; ++i) {
        const auto continuation = static_cast<unsigned char>(text[index + i]);
        if ((continuation & 0xC0) != 0x80) {
            return false;
        }
        value = (value << 6) | (continuation & 0x3F);
    }

    // Reject over-long encodings, surrogates and out-of-range values so they are passed through
    // byte by byte rather than silently re-encoded into a different sequence.
    static constexpr char32_t kMinimum[] = {0, 0, 0x80, 0x800, 0x10000};
    if (value < kMinimum[length] || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) {
        return false;
    }

    index += static_cast<std::size_t>(length);
    codePoint = value;
    return true;
}

void encode(char32_t codePoint, std::string& out) {
    if (codePoint < 0x80) {
        out.push_back(static_cast<char>(codePoint));
    } else if (codePoint < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (codePoint >> 6)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else if (codePoint < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (codePoint >> 12)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (codePoint >> 18)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((codePoint >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (codePoint & 0x3F)));
    }
}

const LowercaseMapping* findMapping(char32_t codePoint) {
    const auto* begin = kLowercaseMappings;
    const auto* end = kLowercaseMappings + kLowercaseMappingCount;

    const auto* hit = std::lower_bound(begin, end, codePoint,
        [](const LowercaseMapping& mapping, char32_t value) { return mapping.upper < value; });

    return (hit != end && hit->upper == codePoint) ? hit : nullptr;
}

}  // namespace

std::string Utf8::toLower(const std::string& text) {
    std::string result;
    result.reserve(text.size());

    std::size_t index = 0;
    while (index < text.size()) {
        const auto byte = static_cast<unsigned char>(text[index]);

        // ASCII is the common case and needs no table lookup.
        if (byte < 0x80) {
            result.push_back(byte >= 'A' && byte <= 'Z' ? static_cast<char>(byte - 'A' + 'a')
                                                        : static_cast<char>(byte));
            ++index;
            continue;
        }

        char32_t codePoint = 0;
        if (!decode(text, index, codePoint)) {
            result.push_back(text[index]);
            ++index;
            continue;
        }

        if (const LowercaseMapping* mapping = findMapping(codePoint)) {
            encode(mapping->first, result);
            if (mapping->second != 0) {
                encode(mapping->second, result);
            }
        } else {
            encode(codePoint, result);
        }
    }

    return result;
}

}  // namespace timetracker::utils
