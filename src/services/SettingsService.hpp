#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <map>
#include <memory>

namespace timetracker::database {
    class Database;
}

namespace timetracker::services {

/**
 * Service for managing application settings stored in the database.
 *
 * Settings storage:
 * - Settings are stored in the database 'settings' table as key-value pairs
 * - Keys: youtrack_url, youtrack_token, activity_aliases (JSON string)
 *
 * Settings schema extensibility:
 * - New settings can be added by defining new keys
 * - All settings operations are designed to be gracefully extensible
 */
class SettingsService {
public:
    explicit SettingsService(std::shared_ptr<timetracker::database::Database> database);
    ~SettingsService() = default;

    // Non-copyable, non-movable
    SettingsService(const SettingsService&) = delete;
    SettingsService& operator=(const SettingsService&) = delete;
    SettingsService(SettingsService&&) = delete;
    SettingsService& operator=(SettingsService&&) = delete;

    /**
     * Load settings from database.
     * Also performs one-time migration from settings.json if needed.
     *
     * @return true if settings were loaded successfully
     */
    bool loadSettings();

    /**
     * Get a setting value by key.
     * @param key The setting key
     * @param defaultValue Default value if key not found
     * @return The setting value or default
     */
    std::string getSetting(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * Set a setting value by key.
     * @param key The setting key
     * @param value The setting value
     */
    void setSetting(const std::string& key, const std::string& value);

    /**
     * Get YouTrack base URL from settings.
     * @return YouTrack URL or empty string if not configured
     */
    std::string getYouTrackUrl() const;

    /**
     * Get YouTrack authentication token from settings.
     * @return YouTrack token or empty string if not configured
     */
    std::string getYouTrackToken() const;

    /**
     * Get activity alias mapping from settings.
     * @return Map of activity names to YouTrack issue IDs
     */
    std::map<std::string, std::string> getActivityAliases() const;

    /**
     * Set YouTrack URL.
     * @param url The YouTrack base URL
     */
    void setYouTrackUrl(const std::string& url);

    /**
     * Set YouTrack token.
     * @param token The YouTrack authentication token
     */
    void setYouTrackToken(const std::string& token);

    /**
     * Set activity aliases.
     * @param aliases Map of activity names to YouTrack issue IDs
     */
    void setActivityAliases(const std::map<std::string, std::string>& aliases);

    /**
     * Get whether to include unplanned meetings in KTalk imports.
     * @return true if unplanned meetings should be included (default), false otherwise
     */
    bool getKTalkIncludeUnplanned() const;

    /**
     * Set whether to include unplanned meetings in KTalk imports.
     * @param include true to include unplanned meetings, false to exclude them
     */
    void setKTalkIncludeUnplanned(bool include);

    /**
     * Get the KTalk import snap interval in minutes.
     * @return Snap interval in minutes, or 0 if snapping is disabled (default)
     */
    int getKTalkSnapInterval() const;

    /**
     * Set the KTalk import snap interval in minutes.
     * @param intervalMinutes Snap interval in minutes (0 = disabled)
     */
    void setKTalkSnapInterval(int intervalMinutes);

    /**
     * Get the UI language preference.
     * @return Language code ("en" or "ru"), defaults to "ru"
     */
    std::string getLanguage() const;

    /**
     * Set the UI language preference.
     * @param language Language code ("en" or "ru")
     */
    void setLanguage(const std::string& language);

    /**
     * Get whether block schedule mode is enabled.
     * @return true if block schedule mode is enabled, false by default
     */
    bool getBlockScheduleEnabled() const;

    /**
     * Set whether block schedule mode is enabled.
     * @param enabled true to enable block schedule mode
     */
    void setBlockScheduleEnabled(bool enabled);

    /**
     * Get the workday start time for block schedule.
     * @return Start time as "HH:MM" string, defaults to "09:00"
     */
    std::string getBlockScheduleWorkdayStart() const;

    /**
     * Set the workday start time for block schedule.
     * @param time Start time as "HH:MM" string
     */
    void setBlockScheduleWorkdayStart(const std::string& time);

    /**
     * Get the workday end time for block schedule.
     * @return End time as "HH:MM" string, defaults to "18:00"
     */
    std::string getBlockScheduleWorkdayEnd() const;

    /**
     * Set the workday end time for block schedule.
     * @param time End time as "HH:MM" string
     */
    void setBlockScheduleWorkdayEnd(const std::string& time);

private:
    std::shared_ptr<timetracker::database::Database> database_;
    std::map<std::string, std::string> settingsCache_;

    /**
     * Load all settings from database into cache.
     */
    void loadFromDatabase();
};

} // namespace timetracker::services
