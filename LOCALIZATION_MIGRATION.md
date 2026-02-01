# Localization System Migration Guide

The localization system has been redesigned to use a simpler key-value approach:

## Old System
```cpp
auto& L = localization::L10n();
L.Common.Save()
L.TimeEntries.Activity()
L.DateTime.HourAbbrev()
```

## New System
```cpp
auto& L = localization::L10n();
L.get("Save")
L.get("Activity")
L.get("h")
```

## Key Changes

1. **English text is the key**: Use the actual English text as the key
2. **No more categories**: No need for `Common`, `TimeEntries`, etc.
3. **Simpler syntax**: Just `L.get("Text")`
4. **Auto-fallback**: If no translation exists, returns the English key

## Integer Values

For integer values like FirstDayOfWeek:
```cpp
// Old
int firstDay = L.DateTime.FirstDayOfWeek();

// New
int firstDay = L.getInt("FirstDayOfWeek", 0);
```

## Complete Transformation Reference

### Common
- `L.Common.Save()` → `L.get("Save")`
- `L.Common.Cancel()` → `L.get("Cancel")`
- `L.Common.OK()` → `L.get("OK")`
- `L.Common.Error()` → `L.get("Error")`
- `L.Common.Success()` → `L.get("Success")`
- `L.Common.Warning()` → `L.get("Warning")`
- `L.Common.Delete()` → `L.get("Delete")`
- `L.Common.Add()` → `L.get("Add")`
- `L.Common.Edit()` → `L.get("Edit")`
- `L.Common.Close()` → `L.get("Close")`
- `L.Common.Yes()` → `L.get("Yes")`
- `L.Common.No()` → `L.get("No")`

### Main Window
- `L.MainWindow.Title()` → `L.get("Time Tracker")`
- `L.MainWindow.Start()` → `L.get("Start")`
- `L.MainWindow.Stop()` → `L.get("Stop")`
- `L.MainWindow.ExportCSV()` → `L.get("Export CSV")`
- `L.MainWindow.ExportToYouTrack()` → `L.get("Export to YouTrack")`
- `L.MainWindow.KTalkImport()` → `L.get("KTalk Import")`
- `L.MainWindow.Settings()` → `L.get("Settings")`

### Date Selector
- `L.DateSelector.From()` → `L.get("From")`
- `L.DateSelector.To()` → `L.get("to")`
- `L.DateSelector.Today()` → `L.get("Today")`
- `L.DateSelector.ThisWeek()` → `L.get("This Week")`
- `L.DateSelector.ThisMonth()` → `L.get("This Month")`

### Time Entries
- `L.TimeEntries.Activity()` → `L.get("Activity")`
- `L.TimeEntries.Description()` → `L.get("Description")`
- `L.TimeEntries.StartTime()` → `L.get("Start")`
- `L.TimeEntries.EndTime()` → `L.get("End")`
- `L.TimeEntries.Duration()` → `L.get("Duration")`
- `L.TimeEntries.Actions()` → `L.get("Actions")`
- `L.TimeEntries.EditActivity()` → `L.get("Edit Activity")`
- `L.TimeEntries.AddActivity()` → `L.get("Add Activity")`
- `L.TimeEntries.DeleteActivity()` → `L.get("Delete Activity")`
- `L.TimeEntries.NoEntries()` → `L.get("No entries for selected date range")`
- `L.TimeEntries.CurrentlyTracking()` → `L.get("Currently tracking")`
- `L.TimeEntries.Total()` → `L.get("Total")`
- `L.TimeEntries.ShowBreakdown()` → `L.get("Show activity breakdown")`
- `L.TimeEntries.HideBreakdown()` → `L.get("Hide activity breakdown")`
- `L.TimeEntries.ActivityNamePlaceholder()` → `L.get("Activity name")`
- `L.TimeEntries.DescriptionPlaceholder()` → `L.get("Optional description")`
- `L.TimeEntries.Ongoing()` → `L.get("(ongoing)")`
- `L.TimeEntries.EditEntry()` → `L.get("Edit Entry")`
- `L.TimeEntries.AddEntry()` → `L.get("Add Entry")`
- `L.TimeEntries.ActivityLabel()` → `L.get("Activity:")`
- `L.TimeEntries.DescriptionLabel()` → `L.get("Description:")`
- `L.TimeEntries.StartLabel()` → `L.get("Start:")`
- `L.TimeEntries.EndLabel()` → `L.get("End:")`
- `L.TimeEntries.OngoingCheckbox()` → `L.get("Ongoing")`

### Settings
- `L.Settings.Title()` → `L.get("Settings")`
- `L.Settings.YouTrackSettings()` → `L.get("YouTrack Settings")`
- `L.Settings.URL()` → `L.get("URL:")`
- `L.Settings.Token()` → `L.get("Token:")`
- `L.Settings.URLTooltip()` → `L.get("YouTrack server URL (e.g., https://youtrack.company.com)")`
- `L.Settings.TokenTooltip()` → `L.get("YouTrack permanent token for authentication")`
- `L.Settings.ExportLog()` → `L.get("Export Log")`
- `L.Settings.ActivityAliases()` → `L.get("Activity Aliases")`
- `L.Settings.MapActivityNames()` → `L.get("Map activity names to YouTrack issue IDs:")`
- `L.Settings.ActivityName()` → `L.get("Activity Name")`
- `L.Settings.IssueID()` → `L.get("Issue ID")`
- `L.Settings.AddAlias()` → `L.get("+ Add Alias")`
- `L.Settings.KTalkImportSettings()` → `L.get("KTalk Import")`
- `L.Settings.IncludeUnplannedMeetings()` → `L.get("Include unplanned meetings")`
- `L.Settings.Language()` → `L.get("Language:")`
- `L.Settings.LanguageEnglish()` → `L.get("English")`
- `L.Settings.LanguageRussian()` → `L.get("Russian")`

### Date/Time Formats
- `L.DateTime.DateFormat()` → `L.get("%m/%d/%Y")`
- `L.DateTime.TimeFormat()` → `L.get("%I:%M %p")`
- `L.DateTime.DateTimeFormat()` → `L.get("%m/%d/%Y %I:%M %p")`
- `L.DateTime.DurationFormat()` → `L.get("%dh %02dm")`
- `L.DateTime.HourAbbrev()` → `L.get("h")`
- `L.DateTime.MinuteAbbrev()` → `L.get("min")`
- `L.DateTime.MonthNames()` → `L.get("January,February,March,April,May,June,July,August,September,October,November,December")`
- `L.DateTime.MonthNamesShort()` → `L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec")`
- `L.DateTime.WeekdayNames()` → `L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat")`
- `L.DateTime.WeekdayNamesShort()` → `L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat")`
- `L.DateTime.FirstDayOfWeek()` → `L.getInt("FirstDayOfWeek", 0)`

### System Tray
- `L.SystemTray.Idle()` → `L.get("Time Tracker - Idle")`
- `L.SystemTray.TrackingPrefix()` → `L.get("Time Tracker - Tracking: ")`
- `L.SystemTray.QuickAddActivity()` → `L.get("Quick Add Activity")`
- `L.SystemTray.ShowWindow()` → `L.get("Show Window")`
- `L.SystemTray.StopTracking()` → `L.get("Stop Tracking")`
- `L.SystemTray.StopTrackingPrefix()` → `L.get("Stop Tracking: ")`
- `L.SystemTray.RecentActivities()` → `L.get("Recent Activities")`
- `L.SystemTray.Exit()` → `L.get("Exit")`
- `L.SystemTray.Tracking()` → `L.get("Tracking: ")`

### Export
- `L.Export.ExportConfirmationTitle()` → `L.get("Export to YouTrack")`
- `L.Export.ExportConfirmationMessage()` → `L.get("Export %d time entries to YouTrack?")`
- `L.Export.Confirm()` → `L.get("Confirm")`
- `L.Export.ExportProgress()` → `L.get("Exporting...")`
- `L.Export.ExportProgressMessage()` → `L.get("Exporting %d of %d entries...")`
- `L.Export.ExportSuccess()` → `L.get("Export Complete")`
- `L.Export.ExportSuccessMessage()` → `L.get("Successfully exported %d entries to YouTrack.")`
- `L.Export.ExportError()` → `L.get("Export Error")`
- `L.Export.ExportErrorMessage()` → `L.get("Failed to export entries:\\n%s")`
- `L.Export.CSVExportSuccess()` → `L.get("CSV exported successfully")`
- `L.Export.CSVExportError()` → `L.get("Failed to export CSV")`

### Errors
- `L.Errors.OverlapTitle()` → `L.get("Time Overlap Detected")`
- `L.Errors.OverlapMessage()` → `L.get("This entry overlaps with an existing entry:\\n\\nExisting: %s (%s - %s)\\n\\nPlease adjust the times to avoid overlap.")`
- `L.Errors.InvalidTimeRange()` → `L.get("End time must be after start time")`
- `L.Errors.ActivityNameRequired()` → `L.get("Activity name is required")`
- `L.Errors.YouTrackNotConfigured()` → `L.get("YouTrack is not configured. Please set URL and token in Settings.")`
- `L.Errors.NoActivityAlias()` → `L.get("No YouTrack issue ID configured for activity: %s")`

### Quick Add
- `L.QuickAdd.Title()` → `L.get("Quick Add Activity")`
- `L.QuickAdd.ActivityName()` → `L.get("Activity:")`
- `L.QuickAdd.Description()` → `L.get("Description:")`
- `L.QuickAdd.StartTracking()` → `L.get("Start Tracking")`

### Export Log
- `L.ExportLog.Title()` → `L.get("YouTrack Export Log")`
- `L.ExportLog.ExportTime()` → `L.get("Export Time")`
- `L.ExportLog.Activity()` → `L.get("Activity")`
- `L.ExportLog.IssueID()` → `L.get("Issue ID")`
- `L.ExportLog.TrackedDate()` → `L.get("Tracked Date")`
- `L.ExportLog.Duration()` → `L.get("Duration")`
- `L.ExportLog.NoEntriesMessage()` → `L.get("No export log entries found. Export some activities to YouTrack to see them logged here.")`
- `L.ExportLog.PreviousPage()` → `L.get("< Previous")`
- `L.ExportLog.NextPage()` → `L.get("Next >")`
- `L.ExportLog.PageIndicator()` → `L.get("Page %d of %d")`
- `L.ExportLog.ClearLog()` → `L.get("Clear Log")`
- `L.ExportLog.ClearLogConfirmTitle()` → `L.get("Clear Log Confirmation")`
- `L.ExportLog.ClearLogConfirmMessage()` → `L.get("Are you sure you want to clear all export log entries?")`
- `L.ExportLog.ClearLogWarning()` → `L.get("This action cannot be undone.")`
- `L.ExportLog.ClearLogAction()` → `L.get("Yes, Clear Log")`

### KTalk Import
- `L.KTalkImport.Title()` → `L.get("Import from KTalk")`
- `L.KTalkImport.Description()` → `L.get("Import conference history from KTalk. Copy the fetch() request from your browser's DevTools Network tab.")`
- `L.KTalkImport.FromDate()` → `L.get("From Date")`
- `L.KTalkImport.ToDate()` → `L.get("To Date")`
- `L.KTalkImport.FetchPayload()` → `L.get("Fetch Payload:")`
- `L.KTalkImport.SelectDate()` → `L.get("Select date to import:")`
- `L.KTalkImport.Import()` → `L.get("Import")`
- `L.KTalkImport.Importing()` → `L.get("Importing...")`
- `L.KTalkImport.ImportSuccess()` → `L.get("Import successful")`
- `L.KTalkImport.ImportSuccessMessage()` → `L.get("Successfully imported %d conferences.")`
- `L.KTalkImport.ImportError()` → `L.get("Import failed")`
- `L.KTalkImport.NoConferences()` → `L.get("No conferences found for the selected date.")`
