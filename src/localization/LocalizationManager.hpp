#pragma once

#include "Language.hpp"
#include <unordered_map>
#include <string>

namespace timetracker::localization {

/**
 * Singleton manager for application localization.
 * Uses English text as keys, returning translations for the current language.
 * If no translation exists, returns the English text (the key itself).
 */
class LocalizationManager {
public:
    // Singleton access
    static LocalizationManager& instance();

    // Get localized string (returns key if no translation found)
    const char* get(const char* key) const;

    // Get integer value (e.g., for FirstDayOfWeek)
    int getInt(const char* key, int defaultValue = 0) const;

    // Set the current language
    void setLanguage(Language lang);

    // Get the current language
    Language getCurrentLanguage() const;

    // Delete copy and move
    LocalizationManager(const LocalizationManager&) = delete;
    LocalizationManager& operator=(const LocalizationManager&) = delete;
    LocalizationManager(LocalizationManager&&) = delete;
    LocalizationManager& operator=(LocalizationManager&&) = delete;

private:
    LocalizationManager();
    ~LocalizationManager() = default;

    Language currentLanguage_ = Language::Russian;
    std::unordered_map<std::string, std::string> translations_;
    std::unordered_map<std::string, int> intValues_;

    void loadLanguage(Language lang);
    void loadRussianTranslations();
};

// Global convenience accessor
inline LocalizationManager& L10n() {
    return LocalizationManager::instance();
}

} // namespace timetracker::localization
