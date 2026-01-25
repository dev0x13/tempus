#pragma once

#include <nlohmann/json.hpp>
#include <filesystem>
#include <string>
#include <optional>

namespace timetracker::services {

/**
 * Service for managing application settings stored in settings.json.
 *
 * Settings file location:
 * - The settings.json file is placed in the same directory as the executable.
 * - This makes settings portable and easy to find for users.
 *
 * Settings schema extensibility:
 * - Currently, the settings schema is empty (just an empty JSON object {}).
 * - New settings can be added in the future by extending this service.
 * - All settings operations are designed to be gracefully extensible.
 */
class SettingsService {
public:
    SettingsService();
    ~SettingsService() = default;

    // Non-copyable, non-movable
    SettingsService(const SettingsService&) = delete;
    SettingsService& operator=(const SettingsService&) = delete;
    SettingsService(SettingsService&&) = delete;
    SettingsService& operator=(SettingsService&&) = delete;

    /**
     * Load settings from settings.json.
     * If the file doesn't exist, creates it with default values.
     * If the file is corrupted, logs a warning and uses defaults.
     *
     * @return true if settings were loaded successfully (or defaults used)
     */
    bool loadSettings();

    /**
     * Save current settings to settings.json.
     * Uses atomic write (temp file + rename) to prevent corruption.
     *
     * @return true if settings were saved successfully
     */
    bool saveSettings();

    /**
     * Get the current settings as a JSON object.
     * @return const reference to the settings JSON
     */
    const nlohmann::json& getSettings() const { return settings_; }

    /**
     * Get the path to the settings file.
     * @return path to settings.json (in executable directory)
     */
    std::filesystem::path getSettingsPath() const;

private:
    nlohmann::json settings_;

    /**
     * Create default settings (empty JSON object for now).
     * @return default settings JSON object
     */
    nlohmann::json createDefaultSettings() const;

    /**
     * Validate settings JSON structure.
     * @param json The JSON to validate
     * @return true if valid, false otherwise
     */
    bool validateSettings(const nlohmann::json& json) const;
};

} // namespace timetracker::services
