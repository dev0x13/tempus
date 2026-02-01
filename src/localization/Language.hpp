#pragma once

#include <string>

namespace timetracker::localization {

enum class Language {
    English,
    Russian
};

inline std::string languageToString(Language lang) {
    switch (lang) {
        case Language::English: return "en";
        case Language::Russian: return "ru";
        default: return "ru";
    }
}

inline Language stringToLanguage(const std::string& str) {
    if (str == "en") return Language::English;
    if (str == "ru") return Language::Russian;
    return Language::Russian; // default
}

} // namespace timetracker::localization
