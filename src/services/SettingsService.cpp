#include "SettingsService.hpp"
#include <fstream>
#include <iostream>

namespace timetracker::services {

SettingsService::SettingsService() : settings_(createDefaultSettings()) {}

bool SettingsService::loadSettings() {
    auto settingsPath = getSettingsPath();

    // If file doesn't exist, create it with defaults
    if (!std::filesystem::exists(settingsPath)) {
        std::cout << "Settings file not found. Creating with defaults at: " << settingsPath << std::endl;
        settings_ = createDefaultSettings();
        return saveSettings();
    }

    // Try to read the file
    try {
        std::ifstream file(settingsPath);
        if (!file.is_open()) {
            std::cerr << "Warning: Failed to open settings file. Using defaults." << std::endl;
            settings_ = createDefaultSettings();
            return true; // Continue with defaults
        }

        nlohmann::json loadedSettings;
        file >> loadedSettings;

        // Validate the loaded settings
        if (!validateSettings(loadedSettings)) {
            std::cerr << "Warning: Invalid settings file format. Using defaults." << std::endl;
            settings_ = createDefaultSettings();
            return true; // Continue with defaults
        }

        settings_ = loadedSettings;
        return true;

    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Warning: Failed to parse settings file (JSON error: " << e.what() << "). Using defaults." << std::endl;
        settings_ = createDefaultSettings();
        return true; // Continue with defaults
    } catch (const std::exception& e) {
        std::cerr << "Warning: Error reading settings file (" << e.what() << "). Using defaults." << std::endl;
        settings_ = createDefaultSettings();
        return true; // Continue with defaults
    }
}

bool SettingsService::saveSettings() {
    auto settingsPath = getSettingsPath();

    try {
        // Ensure the directory exists
        auto parentDir = settingsPath.parent_path();
        if (!parentDir.empty() && !std::filesystem::exists(parentDir)) {
            std::filesystem::create_directories(parentDir);
        }

        // Use atomic write: write to temp file, then rename
        auto tempPath = settingsPath;
        tempPath += ".tmp";

        // Write to temp file
        {
            std::ofstream file(tempPath);
            if (!file.is_open()) {
                std::cerr << "Error: Failed to open temp settings file for writing: " << tempPath << std::endl;
                return false;
            }

            // Pretty-print with 2-space indentation
            file << settings_.dump(2) << std::endl;
        }

        // Rename temp file to actual file (atomic operation)
        std::filesystem::rename(tempPath, settingsPath);
        return true;

    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: Failed to save settings file (" << e.what() << ")" << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: Failed to save settings file (" << e.what() << ")" << std::endl;
        return false;
    }
}

std::filesystem::path SettingsService::getSettingsPath() const {
    // Settings file is placed in the same directory as the executable
    return std::filesystem::current_path() / "settings.json";
}

nlohmann::json SettingsService::createDefaultSettings() const {
    // Default settings with YouTrack configuration
    nlohmann::json defaults = nlohmann::json::object();
    defaults["youtrack"] = {
        {"url", ""},
        {"token", ""},
        {"activityAliases", nlohmann::json::object()}
    };
    return defaults;
}

bool SettingsService::validateSettings(const nlohmann::json& json) const {
    // Check that it's a valid JSON object
    if (!json.is_object()) {
        return false;
    }

    // Validate YouTrack settings structure if present
    if (json.contains("youtrack")) {
        const auto& yt = json["youtrack"];
        if (!yt.is_object()) {
            return false;
        }
        // Check required fields exist and are strings/objects
        if (yt.contains("url") && !yt["url"].is_string()) {
            return false;
        }
        if (yt.contains("token") && !yt["token"].is_string()) {
            return false;
        }
        if (yt.contains("activityAliases") && !yt["activityAliases"].is_object()) {
            return false;
        }
    }

    return true;
}

std::string SettingsService::getYouTrackUrl() const {
    if (settings_.contains("youtrack") && settings_["youtrack"].contains("url")) {
        return settings_["youtrack"]["url"].get<std::string>();
    }
    return "";
}

std::string SettingsService::getYouTrackToken() const {
    if (settings_.contains("youtrack") && settings_["youtrack"].contains("token")) {
        return settings_["youtrack"]["token"].get<std::string>();
    }
    return "";
}

std::map<std::string, std::string> SettingsService::getActivityAliases() const {
    std::map<std::string, std::string> aliases;
    if (settings_.contains("youtrack") && settings_["youtrack"].contains("activityAliases")) {
        const auto& aliasesJson = settings_["youtrack"]["activityAliases"];
        if (aliasesJson.is_object()) {
            for (auto it = aliasesJson.begin(); it != aliasesJson.end(); ++it) {
                if (it.value().is_string()) {
                    aliases[it.key()] = it.value().get<std::string>();
                }
            }
        }
    }
    return aliases;
}

} // namespace timetracker::services
