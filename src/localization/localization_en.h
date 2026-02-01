#pragma once

namespace timetracker::localization::en {

// Common strings
struct CommonStrings {
    static constexpr const char* Save() { return "Save"; }
    static constexpr const char* Cancel() { return "Cancel"; }
    static constexpr const char* OK() { return "OK"; }
    static constexpr const char* Error() { return "Error"; }
    static constexpr const char* Success() { return "Success"; }
    static constexpr const char* Warning() { return "Warning"; }
    static constexpr const char* Delete() { return "Delete"; }
    static constexpr const char* Add() { return "Add"; }
    static constexpr const char* Edit() { return "Edit"; }
    static constexpr const char* Close() { return "Close"; }
    static constexpr const char* Yes() { return "Yes"; }
    static constexpr const char* No() { return "No"; }
};

// Main window / Top buttons
struct MainWindowStrings {
    static constexpr const char* Title() { return "Time Tracker"; }
    static constexpr const char* Start() { return "Start"; }
    static constexpr const char* Stop() { return "Stop"; }
    static constexpr const char* ExportCSV() { return "Export CSV"; }
    static constexpr const char* ExportToYouTrack() { return "Export to YouTrack"; }
    static constexpr const char* KTalkImport() { return "KTalk Import"; }
    static constexpr const char* Settings() { return "Settings"; }
};

// Date selector
struct DateSelectorStrings {
    static constexpr const char* From() { return "From"; }
    static constexpr const char* To() { return "to"; }
    static constexpr const char* Today() { return "Today"; }
    static constexpr const char* ThisWeek() { return "This Week"; }
    static constexpr const char* ThisMonth() { return "This Month"; }
};

// Time entries view
struct TimeEntriesStrings {
    static constexpr const char* Activity() { return "Activity"; }
    static constexpr const char* Description() { return "Description"; }
    static constexpr const char* StartTime() { return "Start"; }
    static constexpr const char* EndTime() { return "End"; }
    static constexpr const char* Duration() { return "Duration"; }
    static constexpr const char* Actions() { return "Actions"; }
    static constexpr const char* EditActivity() { return "Edit Activity"; }
    static constexpr const char* AddActivity() { return "Add Activity"; }
    static constexpr const char* DeleteActivity() { return "Delete Activity"; }
    static constexpr const char* NoEntries() { return "No entries for selected date range"; }
    static constexpr const char* CurrentlyTracking() { return "Currently tracking"; }
    static constexpr const char* Total() { return "Total"; }
    static constexpr const char* ShowBreakdown() { return "Show activity breakdown"; }
    static constexpr const char* HideBreakdown() { return "Hide activity breakdown"; }
    static constexpr const char* ActivityNamePlaceholder() { return "Activity name"; }
    static constexpr const char* DescriptionPlaceholder() { return "Optional description"; }
};

// Settings dialog
struct SettingsStrings {
    static constexpr const char* Title() { return "Settings"; }
    static constexpr const char* YouTrackSettings() { return "YouTrack Settings"; }
    static constexpr const char* URL() { return "URL:"; }
    static constexpr const char* Token() { return "Token:"; }
    static constexpr const char* URLTooltip() { return "YouTrack server URL (e.g., https://youtrack.company.com)"; }
    static constexpr const char* TokenTooltip() { return "YouTrack permanent token for authentication"; }
    static constexpr const char* ExportLog() { return "Export Log"; }
    static constexpr const char* ActivityAliases() { return "Activity Aliases"; }
    static constexpr const char* MapActivityNames() { return "Map activity names to YouTrack issue IDs:"; }
    static constexpr const char* ActivityName() { return "Activity Name"; }
    static constexpr const char* IssueID() { return "Issue ID"; }
    static constexpr const char* AddAlias() { return "+ Add Alias"; }
    static constexpr const char* KTalkImportSettings() { return "KTalk Import"; }
    static constexpr const char* IncludeUnplannedMeetings() { return "Include unplanned meetings"; }
    static constexpr const char* IncludeUnplannedTooltip() { return "When enabled, imports all conferences including those without a title.\nWhen disabled, only imports conferences that have a title."; }
    static constexpr const char* URLValidationError() { return "URL must start with http:// or https://"; }
    static constexpr const char* AliasValidationError() { return "All alias entries must have both activity name and issue ID"; }
    static constexpr const char* Language() { return "Language:"; }
    static constexpr const char* LanguageEnglish() { return "English"; }
    static constexpr const char* LanguageRussian() { return "Russian"; }
};

// System tray
struct SystemTrayStrings {
    static constexpr const char* Idle() { return "Time Tracker - Idle"; }
    static constexpr const char* TrackingPrefix() { return "Time Tracker - Tracking: "; }
    static constexpr const char* QuickAddActivity() { return "Quick Add Activity"; }
    static constexpr const char* ShowWindow() { return "Show Window"; }
    static constexpr const char* StopTracking() { return "Stop Tracking"; }
    static constexpr const char* StopTrackingPrefix() { return "Stop Tracking: "; }
    static constexpr const char* RecentActivities() { return "Recent Activities"; }
    static constexpr const char* Exit() { return "Exit"; }
    static constexpr const char* Tracking() { return "Tracking: "; }
};

// Export dialogs
struct ExportStrings {
    static constexpr const char* ExportConfirmationTitle() { return "Export to YouTrack"; }
    static constexpr const char* ExportConfirmationMessage() { return "Export %d time entries to YouTrack?"; }
    static constexpr const char* Confirm() { return "Confirm"; }
    static constexpr const char* ExportProgress() { return "Exporting..."; }
    static constexpr const char* ExportProgressMessage() { return "Exporting %d of %d entries..."; }
    static constexpr const char* ExportSuccess() { return "Export Complete"; }
    static constexpr const char* ExportSuccessMessage() { return "Successfully exported %d entries to YouTrack."; }
    static constexpr const char* ExportError() { return "Export Error"; }
    static constexpr const char* ExportErrorMessage() { return "Failed to export entries:\n%s"; }
    static constexpr const char* CSVExportSuccess() { return "CSV exported successfully"; }
    static constexpr const char* CSVExportError() { return "Failed to export CSV"; }
};

// Error messages
struct ErrorStrings {
    static constexpr const char* OverlapTitle() { return "Time Overlap Detected"; }
    static constexpr const char* OverlapMessage() { return "This entry overlaps with an existing entry:\n\nExisting: %s (%s - %s)\n\nPlease adjust the times to avoid overlap."; }
    static constexpr const char* InvalidTimeRange() { return "End time must be after start time"; }
    static constexpr const char* ActivityNameRequired() { return "Activity name is required"; }
    static constexpr const char* YouTrackNotConfigured() { return "YouTrack is not configured. Please set URL and token in Settings."; }
    static constexpr const char* NoActivityAlias() { return "No YouTrack issue ID configured for activity: %s"; }
};

// Date and time formats
struct DateTimeFormats {
    static constexpr const char* DateFormat() { return "%m/%d/%Y"; }
    static constexpr const char* TimeFormat() { return "%I:%M %p"; }
    static constexpr const char* DateTimeFormat() { return "%m/%d/%Y %I:%M %p"; }
    static constexpr const char* DurationFormat() { return "%dh %02dm"; }
    static constexpr const char* MonthNames() { return "January,February,March,April,May,June,July,August,September,October,November,December"; }
    static constexpr const char* WeekdayNames() { return "Sun,Mon,Tue,Wed,Thu,Fri,Sat"; }
    static constexpr int FirstDayOfWeek() { return 0; } // Sunday
};

// Quick Add Dialog
struct QuickAddStrings {
    static constexpr const char* Title() { return "Quick Add Activity"; }
    static constexpr const char* ActivityName() { return "Activity:"; }
    static constexpr const char* Description() { return "Description:"; }
    static constexpr const char* StartTracking() { return "Start Tracking"; }
};

// Export Log Window
struct ExportLogStrings {
    static constexpr const char* Title() { return "YouTrack Export Log"; }
    static constexpr const char* Date() { return "Date"; }
    static constexpr const char* Status() { return "Status"; }
    static constexpr const char* Message() { return "Message"; }
    static constexpr const char* NoEntries() { return "No export log entries"; }
    static constexpr const char* StatusSuccess() { return "Success"; }
    static constexpr const char* StatusError() { return "Error"; }
};

// KTalk Import Window
struct KTalkImportStrings {
    static constexpr const char* Title() { return "Import from KTalk"; }
    static constexpr const char* SelectDate() { return "Select date to import:"; }
    static constexpr const char* Import() { return "Import"; }
    static constexpr const char* Importing() { return "Importing..."; }
    static constexpr const char* ImportSuccess() { return "Import successful"; }
    static constexpr const char* ImportSuccessMessage() { return "Successfully imported %d conferences."; }
    static constexpr const char* ImportError() { return "Import failed"; }
    static constexpr const char* NoConferences() { return "No conferences found for the selected date."; }
};

} // namespace timetracker::localization::en
