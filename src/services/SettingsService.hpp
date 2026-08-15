#pragma once

#include "models/AutoFillProfile.hpp"

#include <nlohmann/json.hpp>
#include <cstdint>
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

    /**
     * Get the time of day auto-fill starts filling from.
     * Independent of the block schedule window, which is usually kept wide for manual entry.
     * @return Start time as "HH:MM" string, defaults to "09:00"
     */
    std::string getAutoFillDayStart() const;

    /**
     * Set the time of day auto-fill starts filling from.
     * @param time Start time as "HH:MM" string
     */
    void setAutoFillDayStart(const std::string& time);

    /**
     * Get the amount of time auto-fill should account for on a single day.
     * @return Minutes per day, defaults to 480 (8 hours)
     */
    int getAutoFillAvailableMinutes() const;

    /**
     * Set the amount of time auto-fill should account for on a single day.
     * @param minutes Minutes per day (clamped to at least 1)
     */
    void setAutoFillAvailableMinutes(int minutes);

    /**
     * Get the grid that generated block boundaries snap to.
     * @return Grid step in minutes, defaults to 15
     */
    int getAutoFillGridMinutes() const;

    /**
     * Set the grid that generated block boundaries snap to.
     * @param minutes Grid step in minutes (clamped to 1..60)
     */
    void setAutoFillGridMinutes(int minutes);

    /**
     * Get the shortest block auto-fill is allowed to create.
     * @return Minimum block length in minutes, defaults to 15
     */
    int getAutoFillMinBlockMinutes() const;

    /**
     * Set the shortest block auto-fill is allowed to create.
     * @param minutes Minimum block length in minutes (clamped to at least 1)
     */
    void setAutoFillMinBlockMinutes(int minutes);

    /**
     * Get the saved auto-fill percentage profile.
     * @return Activity/percent pairs in the order the user entered them, empty if unset
     */
    models::AutoFillProfile getAutoFillProfile() const;

    /**
     * Set the auto-fill percentage profile.
     * @param profile Activity/percent pairs; order is preserved
     */
    void setAutoFillProfile(const models::AutoFillProfile& profile);

    /**
     * Get the KTalk space URL, e.g. "https://example.ktalk.ru".
     * The API path is appended by KTalkImportService, not stored here.
     * @return Space URL, or empty string if not configured
     */
    std::string getKTalkSpaceUrl() const;

    /**
     * Set the KTalk space URL.
     * @param url Space URL, scheme and host only
     */
    void setKTalkSpaceUrl(const std::string& url);

    /**
     * Get the saved KTalk session token.
     * @return Token, or empty string if none is stored
     */
    std::string getKTalkToken() const;

    /**
     * Set the KTalk session token.
     * @param token Session token; an empty value clears the stored secret
     */
    void setKTalkToken(const std::string& token);

    /**
     * Get when the KTalk token was last saved.
     * @return Unix timestamp in seconds, or 0 if never saved
     */
    int64_t getKTalkTokenSavedAt() const;

    /**
     * Set when the KTalk token was last saved.
     * @param timestamp Unix timestamp in seconds
     */
    void setKTalkTokenSavedAt(int64_t timestamp);

private:
    std::shared_ptr<timetracker::database::Database> database_;
    std::map<std::string, std::string> settingsCache_;

    /**
     * Load all settings from database into cache.
     */
    void loadFromDatabase();

    /**
     * Read a setting whose stored value is a utils::SecretStore marker.
     * Values written before secrets were encrypted are returned as-is.
     */
    std::string getSecretSetting(const std::string& key, const std::string& defaultValue = "") const;

    /**
     * Write a setting through utils::SecretStore, persisting only the marker.
     * Falls back to storing the raw value if no platform backend accepts it.
     */
    void setSecretSetting(const std::string& key, const std::string& value);

    /**
     * Re-write any secret still held as plaintext through utils::SecretStore.
     * Idempotent; runs once per start from loadSettings().
     */
    void migratePlaintextSecrets();

    /**
     * Convert a connection captured from a pasted fetch() request into the space URL the
     * app now builds its own requests from. Idempotent; runs once per start.
     */
    void migrateKTalkConnection();
};

} // namespace timetracker::services
