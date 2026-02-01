#include "LocalizationManager.hpp"
#include "localization_ru.h"

namespace timetracker::localization {

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager instance;
    return instance;
}

LocalizationManager::LocalizationManager() {
    loadLanguage(currentLanguage_);
}

const char* LocalizationManager::get(const char* key) const {
    if (currentLanguage_ == Language::English) {
        // For English, return the key itself
        return key;
    }

    // For other languages, look up in translations map
    auto it = translations_.find(key);
    if (it != translations_.end()) {
        return it->second.c_str();
    }

    // If no translation found, return the key (English text)
    return key;
}

int LocalizationManager::getInt(const char* key, int defaultValue) const {
    if (currentLanguage_ == Language::English) {
        // For English, return default value (English settings)
        if (std::string(key) == "FirstDayOfWeek") {
            return 0; // Sunday
        }
        return defaultValue;
    }

    // For other languages, look up in int values map
    auto it = intValues_.find(key);
    if (it != intValues_.end()) {
        return it->second;
    }

    return defaultValue;
}

void LocalizationManager::setLanguage(Language lang) {
    if (currentLanguage_ != lang) {
        currentLanguage_ = lang;
        loadLanguage(lang);
    }
}

Language LocalizationManager::getCurrentLanguage() const {
    return currentLanguage_;
}

void LocalizationManager::loadLanguage(Language lang) {
    translations_.clear();
    intValues_.clear();

    if (lang == Language::Russian) {
        loadRussianTranslations();
    }
    // For English, we don't need to load anything - get() returns the key
}

void LocalizationManager::loadRussianTranslations() {
    translations_ = ru::getTranslations();
    intValues_ = ru::getIntValues();
}

} // namespace timetracker::localization
