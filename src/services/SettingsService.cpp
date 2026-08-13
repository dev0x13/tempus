#include "SettingsService.hpp"
#include "../database/Database.hpp"
#include "utils/SecretStore.hpp"
#include <iostream>
#include <SQLiteCpp/SQLiteCpp.h>

namespace timetracker::services {

SettingsService::SettingsService(std::shared_ptr<timetracker::database::Database> database)
    : database_(database) {}

bool SettingsService::loadSettings() {
    try {
        loadFromDatabase();
        migratePlaintextSecrets();
        migrateKTalkConnection();
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

std::string SettingsService::getSecretSetting(const std::string& key, const std::string& defaultValue) const {
    auto it = settingsCache_.find(key);
    if (it == settingsCache_.end() || it->second.empty()) {
        return defaultValue;
    }
    return utils::SecretStore::retrieve(key, it->second);
}

void SettingsService::setSecretSetting(const std::string& key, const std::string& value) {
    if (value.empty()) {
        utils::SecretStore::erase(key);
        setSetting(key, "");
        return;
    }

    if (auto marker = utils::SecretStore::store(key, value)) {
        setSetting(key, *marker);
        return;
    }

    std::cerr << "Warning: no secret backend accepted '" << key
              << "', storing it as plaintext" << std::endl;
    setSetting(key, value);
}

void SettingsService::migratePlaintextSecrets() {
    // Databases written before secrets were encrypted hold the raw value in the
    // settings row; re-writing it through SecretStore replaces it with a marker.
    static const char* const kSecretKeys[] = {"youtrack_token", "ktalk_token"};

    for (const char* key : kSecretKeys) {
        auto it = settingsCache_.find(key);
        if (it == settingsCache_.end() || it->second.empty()) {
            continue;
        }
        if (utils::SecretStore::isMarker(it->second)) {
            continue;
        }
        setSecretSetting(key, it->second);
    }
}

void SettingsService::migrateKTalkConnection() {
    // Earlier builds captured a whole endpoint URL plus a header set from a pasted
    // fetch() request. Only the space URL is needed now — the path and the single
    // Authorization header are built by KTalkImportService.
    if (!getSetting("ktalk_space_url").empty()) {
        return;
    }

    const std::string legacyUrl = getSetting("ktalk_base_url");
    if (legacyUrl.empty()) {
        return;
    }

    const size_t schemeEnd = legacyUrl.find("://");
    const size_t hostStart = (schemeEnd == std::string::npos) ? 0 : schemeEnd + 3;
    const size_t hostEnd = legacyUrl.find('/', hostStart);
    const std::string spaceUrl =
        (hostEnd == std::string::npos) ? legacyUrl : legacyUrl.substr(0, hostEnd);

    setSetting("ktalk_space_url", spaceUrl);
    setSetting("ktalk_base_url", "");

    utils::SecretStore::erase("ktalk_headers");
    setSetting("ktalk_headers", "");
}

std::string SettingsService::getYouTrackUrl() const {
    return getSetting("youtrack_url", "");
}

std::string SettingsService::getYouTrackToken() const {
    return getSecretSetting("youtrack_token", "");
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
    setSecretSetting("youtrack_token", token);
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

int SettingsService::getKTalkSnapInterval() const {
    std::string value = getSetting("ktalk_snap_interval", "0");
    try {
        int v = std::stoi(value);
        return v < 0 ? 0 : v;
    } catch (...) {
        return 0;
    }
}

void SettingsService::setKTalkSnapInterval(int intervalMinutes) {
    setSetting("ktalk_snap_interval", std::to_string(intervalMinutes < 0 ? 0 : intervalMinutes));
}

std::string SettingsService::getKTalkSpaceUrl() const {
    return getSetting("ktalk_space_url", "");
}

void SettingsService::setKTalkSpaceUrl(const std::string& url) {
    setSetting("ktalk_space_url", url);
}

std::string SettingsService::getKTalkToken() const {
    return getSecretSetting("ktalk_token", "");
}

void SettingsService::setKTalkToken(const std::string& token) {
    setSecretSetting("ktalk_token", token);
}

int64_t SettingsService::getKTalkTokenSavedAt() const {
    try {
        return std::stoll(getSetting("ktalk_token_saved_at", "0"));
    } catch (...) {
        return 0;
    }
}

void SettingsService::setKTalkTokenSavedAt(int64_t timestamp) {
    setSetting("ktalk_token_saved_at", std::to_string(timestamp));
}

std::string SettingsService::getLanguage() const {
    return getSetting("language", "ru");
}

void SettingsService::setLanguage(const std::string& language) {
    setSetting("language", language);
}

bool SettingsService::getBlockScheduleEnabled() const {
    return getSetting("block_schedule_enabled", "true") == "true";
}

void SettingsService::setBlockScheduleEnabled(bool enabled) {
    setSetting("block_schedule_enabled", enabled ? "true" : "false");
}


std::string SettingsService::getBlockScheduleWorkdayStart() const {
    return getSetting("block_schedule_workday_start", "09:00");
}

void SettingsService::setBlockScheduleWorkdayStart(const std::string& time) {
    setSetting("block_schedule_workday_start", time);
}

std::string SettingsService::getBlockScheduleWorkdayEnd() const {
    return getSetting("block_schedule_workday_end", "18:00");
}

void SettingsService::setBlockScheduleWorkdayEnd(const std::string& time) {
    setSetting("block_schedule_workday_end", time);
}

namespace {

// Shared shape of the numeric auto-fill settings: parse, fall back on garbage, clamp.
int readClampedInt(const std::string& value, int defaultValue, int minValue, int maxValue) {
    int parsed = defaultValue;
    try {
        parsed = std::stoi(value);
    } catch (...) {
        return defaultValue;
    }
    if (parsed < minValue) return minValue;
    if (parsed > maxValue) return maxValue;
    return parsed;
}

int clampInt(int value, int minValue, int maxValue) {
    if (value < minValue) return minValue;
    if (value > maxValue) return maxValue;
    return value;
}

} // namespace

std::string SettingsService::getAutoFillDayStart() const {
    return getSetting("autofill_day_start", "09:00");
}

void SettingsService::setAutoFillDayStart(const std::string& time) {
    setSetting("autofill_day_start", time);
}

int SettingsService::getAutoFillAvailableMinutes() const {
    return readClampedInt(getSetting("autofill_available_minutes", "480"), 480, 1, 24 * 60);
}

void SettingsService::setAutoFillAvailableMinutes(int minutes) {
    setSetting("autofill_available_minutes", std::to_string(clampInt(minutes, 1, 24 * 60)));
}

int SettingsService::getAutoFillGridMinutes() const {
    return readClampedInt(getSetting("autofill_grid_minutes", "15"), 15, 1, 60);
}

void SettingsService::setAutoFillGridMinutes(int minutes) {
    setSetting("autofill_grid_minutes", std::to_string(clampInt(minutes, 1, 60)));
}

int SettingsService::getAutoFillMinBlockMinutes() const {
    return readClampedInt(getSetting("autofill_min_block_minutes", "15"), 15, 1, 24 * 60);
}

void SettingsService::setAutoFillMinBlockMinutes(int minutes) {
    setSetting("autofill_min_block_minutes", std::to_string(clampInt(minutes, 1, 24 * 60)));
}

models::AutoFillProfile SettingsService::getAutoFillProfile() const {
    models::AutoFillProfile profile;
    std::string profileJson = getSetting("autofill_profile", "[]");

    // A JSON array (not an object like activity_aliases) because row order is meaningful:
    // it decides who wins ties when allocating and who absorbs the rounding remainder.
    try {
        nlohmann::json j = nlohmann::json::parse(profileJson);
        if (j.is_array()) {
            for (const auto& item : j) {
                if (!item.is_object()) {
                    continue;
                }
                models::AutoFillAllocation allocation;
                if (item.contains("activity") && item["activity"].is_string()) {
                    allocation.activityName = item["activity"].get<std::string>();
                }
                if (item.contains("percent") && item["percent"].is_number_integer()) {
                    allocation.percent = item["percent"].get<int>();
                }
                if (!allocation.activityName.empty()) {
                    profile.push_back(allocation);
                }
            }
        }
    } catch (const nlohmann::json::exception& e) {
        std::cerr << "Warning: Failed to parse auto-fill profile JSON: " << e.what() << std::endl;
    }

    return profile;
}

void SettingsService::setAutoFillProfile(const models::AutoFillProfile& profile) {
    nlohmann::json j = nlohmann::json::array();
    for (const auto& allocation : profile) {
        j.push_back({{"activity", allocation.activityName}, {"percent", allocation.percent}});
    }
    setSetting("autofill_profile", j.dump());
}

} // namespace timetracker::services
