#include "SettingsService.hpp"
#include "../database/Database.hpp"
#include <fstream>
#include <iostream>
#include <ghc/filesystem.hpp>
namespace fs = ghc::filesystem;
#include <SQLiteCpp/SQLiteCpp.h>

namespace timetracker::services {

SettingsService::SettingsService(std::shared_ptr<timetracker::database::Database> database)
    : database_(database) {}

bool SettingsService::loadSettings() {
    try {
        loadFromDatabase();

        // Check if settings are empty (first run after migration to database)
        if (settingsCache_.empty()) {
            // Attempt to migrate from settings.json if it exists
            if (!migrateFromJsonFile()) {
                // Migration failed or not needed, ensure defaults exist
                std::cout << "Using default empty settings" << std::endl;
            }
            // Reload after migration
            loadFromDatabase();
        }

        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading settings: " << e.what() << std::endl;
        return false;
    }
}

void SettingsService::loadFromDatabase() {
    settingsCache_.clear();

    try {
        SQLite::Statement query(database_->getHandle(), "SELECT key, value FROM settings");

        while (query.executeStep()) {
            std::string key = query.getColumn(0).getString();
            std::string value = query.getColumn(1).getString();
            settingsCache_[key] = value;
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Failed to load settings from database: " << e.what() << std::endl;
    }
}

std::string SettingsService::getSetting(const std::string& key, const std::string& defaultValue) const {
    auto it = settingsCache_.find(key);
    if (it != settingsCache_.end()) {
        return it->second;
    }
    return defaultValue;
}

void SettingsService::setSetting(const std::string& key, const std::string& value) {
    try {
        SQLite::Statement stmt(database_->getHandle(), "INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
        stmt.bind(1, key);
        stmt.bind(2, value);
        stmt.exec();

        // Update cache
        settingsCache_[key] = value;
    } catch (const std::exception& e) {
        std::cerr << "Error saving setting '" << key << "': " << e.what() << std::endl;
    }
}

std::string SettingsService::getYouTrackUrl() const {
    return getSetting("youtrack_url", "");
}

std::string SettingsService::getYouTrackToken() const {
    return getSetting("youtrack_token", "");
}

std::map<std::string, std::string> SettingsService::getActivityAliases() const {
    std::map<std::string, std::string> aliases;
    std::string aliasesJson = getSetting("activity_aliases", "{}");

    try {
        nlohmann::json j = nlohmann::json::parse(aliasesJson);
        if (j.is_object()) {
            for (auto it = j.begin(); it != j.end(); ++it) {
                if (it.value().is_string()) {
                    aliases[it.key()] = it.value().get<std::string>();
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Warning: Failed to parse activity aliases JSON: " << e.what() << std::endl;
    }

    return aliases;
}

void SettingsService::setYouTrackUrl(const std::string& url) {
    setSetting("youtrack_url", url);
}

void SettingsService::setYouTrackToken(const std::string& token) {
    setSetting("youtrack_token", token);
}

void SettingsService::setActivityAliases(const std::map<std::string, std::string>& aliases) {
    nlohmann::json j = nlohmann::json::object();
    for (const auto& [key, value] : aliases) {
        j[key] = value;
    }
    setSetting("activity_aliases", j.dump());
}

bool SettingsService::getKTalkIncludeUnplanned() const {
    std::string value = getSetting("ktalk_include_unplanned", "true");
    return value == "true";
}

void SettingsService::setKTalkIncludeUnplanned(bool include) {
    setSetting("ktalk_include_unplanned", include ? "true" : "false");
}

std::string SettingsService::getLanguage() const {
    return getSetting("language", "ru");
}

void SettingsService::setLanguage(const std::string& language) {
    setSetting("language", language);
}

bool SettingsService::migrateFromJsonFile() {
    fs::path settingsPath = fs::current_path() / "settings.json";

    // If file doesn't exist, nothing to migrate
    if (!fs::exists(settingsPath)) {
        std::cout << "No settings.json file found, skipping migration" << std::endl;
        return false;
    }

    try {
        std::cout << "Migrating settings from settings.json to database..." << std::endl;

        std::ifstream file(settingsPath);
        if (!file.is_open()) {
            std::cerr << "Warning: Failed to open settings.json for migration" << std::endl;
            return false;
        }

        nlohmann::json settings;
        file >> settings;

        // Extract YouTrack settings
        if (settings.contains("youtrack") && settings["youtrack"].is_object()) {
            const auto& yt = settings["youtrack"];

            if (yt.contains("url") && yt["url"].is_string()) {
                setYouTrackUrl(yt["url"].get<std::string>());
            }

            if (yt.contains("token") && yt["token"].is_string()) {
                setYouTrackToken(yt["token"].get<std::string>());
            }

            if (yt.contains("activityAliases") && yt["activityAliases"].is_object()) {
                std::map<std::string, std::string> aliases;
                for (auto it = yt["activityAliases"].begin(); it != yt["activityAliases"].end(); ++it) {
                    if (it.value().is_string()) {
                        aliases[it.key()] = it.value().get<std::string>();
                    }
                }
                setActivityAliases(aliases);
            }
        }

        std::cout << "Settings migration completed successfully" << std::endl;
        return true;

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Warning: Failed to parse settings.json during migration (JSON error: " << e.what() << ")" << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Warning: Error during settings migration (" << e.what() << ")" << std::endl;
        return false;
    }
}

} // namespace timetracker::services
