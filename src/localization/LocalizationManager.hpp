#pragma once

#include "Language.hpp"
#include <memory>
#include <functional>

namespace timetracker::localization {

/**
 * Singleton manager for application localization.
 * Provides access to localized strings based on the current language setting.
 */
class LocalizationManager {
public:
    // String function pointer type
    using StringFunc = const char* (*)();
    using IntFunc = int (*)();

    // String groups matching the localization headers
    struct CommonStrings {
        StringFunc Save;
        StringFunc Cancel;
        StringFunc OK;
        StringFunc Error;
        StringFunc Success;
        StringFunc Warning;
        StringFunc Delete;
        StringFunc Add;
        StringFunc Edit;
        StringFunc Close;
        StringFunc Yes;
        StringFunc No;
    } Common;

    struct MainWindowStrings {
        StringFunc Title;
        StringFunc Start;
        StringFunc Stop;
        StringFunc ExportCSV;
        StringFunc ExportToYouTrack;
        StringFunc KTalkImport;
        StringFunc Settings;
    } MainWindow;

    struct DateSelectorStrings {
        StringFunc From;
        StringFunc To;
        StringFunc Today;
        StringFunc ThisWeek;
        StringFunc ThisMonth;
    } DateSelector;

    struct TimeEntriesStrings {
        StringFunc Activity;
        StringFunc Description;
        StringFunc StartTime;
        StringFunc EndTime;
        StringFunc Duration;
        StringFunc Actions;
        StringFunc EditActivity;
        StringFunc AddActivity;
        StringFunc DeleteActivity;
        StringFunc NoEntries;
        StringFunc CurrentlyTracking;
        StringFunc Total;
        StringFunc ShowBreakdown;
        StringFunc HideBreakdown;
        StringFunc ActivityNamePlaceholder;
        StringFunc DescriptionPlaceholder;
    } TimeEntries;

    struct SettingsStrings {
        StringFunc Title;
        StringFunc YouTrackSettings;
        StringFunc URL;
        StringFunc Token;
        StringFunc URLTooltip;
        StringFunc TokenTooltip;
        StringFunc ExportLog;
        StringFunc ActivityAliases;
        StringFunc MapActivityNames;
        StringFunc ActivityName;
        StringFunc IssueID;
        StringFunc AddAlias;
        StringFunc KTalkImportSettings;
        StringFunc IncludeUnplannedMeetings;
        StringFunc IncludeUnplannedTooltip;
        StringFunc URLValidationError;
        StringFunc AliasValidationError;
        StringFunc Language;
        StringFunc LanguageEnglish;
        StringFunc LanguageRussian;
    } Settings;

    struct SystemTrayStrings {
        StringFunc Idle;
        StringFunc TrackingPrefix;
        StringFunc QuickAddActivity;
        StringFunc ShowWindow;
        StringFunc StopTracking;
        StringFunc StopTrackingPrefix;
        StringFunc RecentActivities;
        StringFunc Exit;
        StringFunc Tracking;
    } SystemTray;

    struct ExportStrings {
        StringFunc ExportConfirmationTitle;
        StringFunc ExportConfirmationMessage;
        StringFunc Confirm;
        StringFunc ExportProgress;
        StringFunc ExportProgressMessage;
        StringFunc ExportSuccess;
        StringFunc ExportSuccessMessage;
        StringFunc ExportError;
        StringFunc ExportErrorMessage;
        StringFunc CSVExportSuccess;
        StringFunc CSVExportError;
    } Export;

    struct ErrorStrings {
        StringFunc OverlapTitle;
        StringFunc OverlapMessage;
        StringFunc InvalidTimeRange;
        StringFunc ActivityNameRequired;
        StringFunc YouTrackNotConfigured;
        StringFunc NoActivityAlias;
    } Errors;

    struct DateTimeFormats {
        StringFunc DateFormat;
        StringFunc TimeFormat;
        StringFunc DateTimeFormat;
        StringFunc DurationFormat;
        StringFunc MonthNames;
        StringFunc WeekdayNames;
        IntFunc FirstDayOfWeek;
    } DateTime;

    struct QuickAddStrings {
        StringFunc Title;
        StringFunc ActivityName;
        StringFunc Description;
        StringFunc StartTracking;
    } QuickAdd;

    struct ExportLogStrings {
        StringFunc Title;
        StringFunc Date;
        StringFunc Status;
        StringFunc Message;
        StringFunc NoEntries;
        StringFunc StatusSuccess;
        StringFunc StatusError;
    } ExportLog;

    struct KTalkImportStrings {
        StringFunc Title;
        StringFunc SelectDate;
        StringFunc Import;
        StringFunc Importing;
        StringFunc ImportSuccess;
        StringFunc ImportSuccessMessage;
        StringFunc ImportError;
        StringFunc NoConferences;
    } KTalkImport;

    // Singleton access
    static LocalizationManager& instance();

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

    void loadLanguage(Language lang);
};

// Global convenience accessor
inline LocalizationManager& L10n() {
    return LocalizationManager::instance();
}

} // namespace timetracker::localization
