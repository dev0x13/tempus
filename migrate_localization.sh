#!/bin/bash

# Script to migrate localization calls from old system to new system
# Usage: ./migrate_localization.sh <file>

if [ $# -eq 0 ]; then
    echo "Usage: $0 <file_to_migrate>"
    echo "Example: $0 src/ui/views/TimeEntriesView.cpp"
    exit 1
fi

FILE="$1"

if [ ! -f "$FILE" ]; then
    echo "Error: File '$FILE' not found"
    exit 1
fi

echo "Migrating $FILE..."

# Create backup
cp "$FILE" "$FILE.bak"

# Common transformations
sed -i 's/L\.Common\.Save()/L.get("Save")/g' "$FILE"
sed -i 's/L\.Common\.Cancel()/L.get("Cancel")/g' "$FILE"
sed -i 's/L\.Common\.OK()/L.get("OK")/g' "$FILE"
sed -i 's/L\.Common\.Error()/L.get("Error")/g' "$FILE"
sed -i 's/L\.Common\.Success()/L.get("Success")/g' "$FILE"
sed -i 's/L\.Common\.Warning()/L.get("Warning")/g' "$FILE"
sed -i 's/L\.Common\.Delete()/L.get("Delete")/g' "$FILE"
sed -i 's/L\.Common\.Add()/L.get("Add")/g' "$FILE"
sed -i 's/L\.Common\.Edit()/L.get("Edit")/g' "$FILE"
sed -i 's/L\.Common\.Close()/L.get("Close")/g' "$FILE"
sed -i 's/L\.Common\.Yes()/L.get("Yes")/g' "$FILE"
sed -i 's/L\.Common\.No()/L.get("No")/g' "$FILE"

# Main Window
sed -i 's/L\.MainWindow\.Title()/L.get("Time Tracker")/g' "$FILE"
sed -i 's/L\.MainWindow\.Start()/L.get("Start")/g' "$FILE"
sed -i 's/L\.MainWindow\.Stop()/L.get("Stop")/g' "$FILE"
sed -i 's/L\.MainWindow\.ExportCSV()/L.get("Export CSV")/g' "$FILE"
sed -i 's/L\.MainWindow\.ExportToYouTrack()/L.get("Export to YouTrack")/g' "$FILE"
sed -i 's/L\.MainWindow\.KTalkImport()/L.get("KTalk Import")/g' "$FILE"
sed -i 's/L\.MainWindow\.Settings()/L.get("Settings")/g' "$FILE"

# Date Selector
sed -i 's/L\.DateSelector\.From()/L.get("From")/g' "$FILE"
sed -i 's/L\.DateSelector\.To()/L.get("to")/g' "$FILE"
sed -i 's/L\.DateSelector\.Today()/L.get("Today")/g' "$FILE"
sed -i 's/L\.DateSelector\.ThisWeek()/L.get("This Week")/g' "$FILE"
sed -i 's/L\.DateSelector\.ThisMonth()/L.get("This Month")/g' "$FILE"

# Time Entries
sed -i 's/L\.TimeEntries\.Activity()/L.get("Activity")/g' "$FILE"
sed -i 's/L\.TimeEntries\.Description()/L.get("Description")/g' "$FILE"
sed -i 's/L\.TimeEntries\.StartTime()/L.get("Start")/g' "$FILE"
sed -i 's/L\.TimeEntries\.EndTime()/L.get("End")/g' "$FILE"
sed -i 's/L\.TimeEntries\.Duration()/L.get("Duration")/g' "$FILE"
sed -i 's/L\.TimeEntries\.Actions()/L.get("Actions")/g' "$FILE"
sed -i 's/L\.TimeEntries\.EditActivity()/L.get("Edit Activity")/g' "$FILE"
sed -i 's/L\.TimeEntries\.AddActivity()/L.get("Add Activity")/g' "$FILE"
sed -i 's/L\.TimeEntries\.DeleteActivity()/L.get("Delete Activity")/g' "$FILE"
sed -i 's/L\.TimeEntries\.NoEntries()/L.get("No entries for selected date range")/g' "$FILE"
sed -i 's/L\.TimeEntries\.CurrentlyTracking()/L.get("Currently tracking")/g' "$FILE"
sed -i 's/L\.TimeEntries\.Total()/L.get("Total")/g' "$FILE"
sed -i 's/L\.TimeEntries\.ShowBreakdown()/L.get("Show activity breakdown")/g' "$FILE"
sed -i 's/L\.TimeEntries\.HideBreakdown()/L.get("Hide activity breakdown")/g' "$FILE"
sed -i 's/L\.TimeEntries\.ActivityNamePlaceholder()/L.get("Activity name")/g' "$FILE"
sed -i 's/L\.TimeEntries\.DescriptionPlaceholder()/L.get("Optional description")/g' "$FILE"
sed -i 's/L\.TimeEntries\.Ongoing()/L.get("(ongoing)")/g' "$FILE"
sed -i 's/L\.TimeEntries\.EditEntry()/L.get("Edit Entry")/g' "$FILE"
sed -i 's/L\.TimeEntries\.AddEntry()/L.get("Add Entry")/g' "$FILE"
sed -i 's/L\.TimeEntries\.ActivityLabel()/L.get("Activity:")/g' "$FILE"
sed -i 's/L\.TimeEntries\.DescriptionLabel()/L.get("Description:")/g' "$FILE"
sed -i 's/L\.TimeEntries\.StartLabel()/L.get("Start:")/g' "$FILE"
sed -i 's/L\.TimeEntries\.EndLabel()/L.get("End:")/g' "$FILE"
sed -i 's/L\.TimeEntries\.OngoingCheckbox()/L.get("Ongoing")/g' "$FILE"

# Settings
sed -i 's/L\.Settings\.Title()/L.get("Settings")/g' "$FILE"
sed -i 's/L\.Settings\.YouTrackSettings()/L.get("YouTrack Settings")/g' "$FILE"
sed -i 's/L\.Settings\.URL()/L.get("URL:")/g' "$FILE"
sed -i 's/L\.Settings\.Token()/L.get("Token:")/g' "$FILE"
sed -i 's/L\.Settings\.URLTooltip()/L.get("YouTrack server URL (e.g., https:\/\/youtrack.company.com)")/g' "$FILE"
sed -i 's/L\.Settings\.TokenTooltip()/L.get("YouTrack permanent token for authentication")/g' "$FILE"
sed -i 's/L\.Settings\.ExportLog()/L.get("Export Log")/g' "$FILE"
sed -i 's/L\.Settings\.ActivityAliases()/L.get("Activity Aliases")/g' "$FILE"
sed -i 's/L\.Settings\.MapActivityNames()/L.get("Map activity names to YouTrack issue IDs:")/g' "$FILE"
sed -i 's/L\.Settings\.ActivityName()/L.get("Activity Name")/g' "$FILE"
sed -i 's/L\.Settings\.IssueID()/L.get("Issue ID")/g' "$FILE"
sed -i 's/L\.Settings\.AddAlias()/L.get("+ Add Alias")/g' "$FILE"
sed -i 's/L\.Settings\.KTalkImportSettings()/L.get("KTalk Import")/g' "$FILE"
sed -i 's/L\.Settings\.IncludeUnplannedMeetings()/L.get("Include unplanned meetings")/g' "$FILE"
sed -i 's/L\.Settings\.Language()/L.get("Language:")/g' "$FILE"
sed -i 's/L\.Settings\.LanguageEnglish()/L.get("English")/g' "$FILE"
sed -i 's/L\.Settings\.LanguageRussian()/L.get("Russian")/g' "$FILE"

# DateTime
sed -i 's/L\.DateTime\.DateFormat()/L.get("%m\/%d\/%Y")/g' "$FILE"
sed -i 's/L\.DateTime\.TimeFormat()/L.get("%I:%M %p")/g' "$FILE"
sed -i 's/L\.DateTime\.DateTimeFormat()/L.get("%m\/%d\/%Y %I:%M %p")/g' "$FILE"
sed -i 's/L\.DateTime\.DurationFormat()/L.get("%dh %02dm")/g' "$FILE"
sed -i 's/L\.DateTime\.HourAbbrev()/L.get("h")/g' "$FILE"
sed -i 's/L\.DateTime\.MinuteAbbrev()/L.get("min")/g' "$FILE"
sed -i 's/L\.DateTime\.MonthNames()/L.get("January,February,March,April,May,June,July,August,September,October,November,December")/g' "$FILE"
sed -i 's/L\.DateTime\.MonthNamesShort()/L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec")/g' "$FILE"
sed -i 's/L\.DateTime\.WeekdayNames()/L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat")/g' "$FILE"
sed -i 's/L\.DateTime\.WeekdayNamesShort()/L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat")/g' "$FILE"
sed -i 's/L\.DateTime\.FirstDayOfWeek()/L.getInt("FirstDayOfWeek", 0)/g' "$FILE"

# System Tray
sed -i 's/L\.SystemTray\.Idle()/L.get("Time Tracker - Idle")/g' "$FILE"
sed -i 's/L\.SystemTray\.TrackingPrefix()/L.get("Time Tracker - Tracking: ")/g' "$FILE"
sed -i 's/L\.SystemTray\.QuickAddActivity()/L.get("Quick Add Activity")/g' "$FILE"
sed -i 's/L\.SystemTray\.ShowWindow()/L.get("Show Window")/g' "$FILE"
sed -i 's/L\.SystemTray\.StopTracking()/L.get("Stop Tracking")/g' "$FILE"
sed -i 's/L\.SystemTray\.StopTrackingPrefix()/L.get("Stop Tracking: ")/g' "$FILE"
sed -i 's/L\.SystemTray\.RecentActivities()/L.get("Recent Activities")/g' "$FILE"
sed -i 's/L\.SystemTray\.Exit()/L.get("Exit")/g' "$FILE"
sed -i 's/L\.SystemTray\.Tracking()/L.get("Tracking: ")/g' "$FILE"

# Export
sed -i 's/L\.Export\.ExportConfirmationTitle()/L.get("Export to YouTrack")/g' "$FILE"
sed -i 's/L\.Export\.ExportConfirmationMessage()/L.get("Export %d time entries to YouTrack?")/g' "$FILE"
sed -i 's/L\.Export\.Confirm()/L.get("Confirm")/g' "$FILE"
sed -i 's/L\.Export\.ExportProgress()/L.get("Exporting...")/g' "$FILE"
sed -i 's/L\.Export\.ExportProgressMessage()/L.get("Exporting %d of %d entries...")/g' "$FILE"
sed -i 's/L\.Export\.ExportSuccess()/L.get("Export Complete")/g' "$FILE"
sed -i 's/L\.Export\.ExportSuccessMessage()/L.get("Successfully exported %d entries to YouTrack.")/g' "$FILE"
sed -i 's/L\.Export\.ExportError()/L.get("Export Error")/g' "$FILE"
sed -i 's/L\.Export\.CSVExportSuccess()/L.get("CSV exported successfully")/g' "$FILE"
sed -i 's/L\.Export\.CSVExportError()/L.get("Failed to export CSV")/g' "$FILE"

# Errors
sed -i 's/L\.Errors\.OverlapTitle()/L.get("Time Overlap Detected")/g' "$FILE"
sed -i 's/L\.Errors\.InvalidTimeRange()/L.get("End time must be after start time")/g' "$FILE"
sed -i 's/L\.Errors\.ActivityNameRequired()/L.get("Activity name is required")/g' "$FILE"
sed -i 's/L\.Errors\.YouTrackNotConfigured()/L.get("YouTrack is not configured. Please set URL and token in Settings.")/g' "$FILE"

# QuickAdd
sed -i 's/L\.QuickAdd\.Title()/L.get("Quick Add Activity")/g' "$FILE"
sed -i 's/L\.QuickAdd\.ActivityName()/L.get("Activity:")/g' "$FILE"
sed -i 's/L\.QuickAdd\.Description()/L.get("Description:")/g' "$FILE"
sed -i 's/L\.QuickAdd\.StartTracking()/L.get("Start Tracking")/g' "$FILE"

# ExportLog
sed -i 's/L\.ExportLog\.Title()/L.get("YouTrack Export Log")/g' "$FILE"
sed -i 's/L\.ExportLog\.ExportTime()/L.get("Export Time")/g' "$FILE"
sed -i 's/L\.ExportLog\.Activity()/L.get("Activity")/g' "$FILE"
sed -i 's/L\.ExportLog\.IssueID()/L.get("Issue ID")/g' "$FILE"
sed -i 's/L\.ExportLog\.TrackedDate()/L.get("Tracked Date")/g' "$FILE"
sed -i 's/L\.ExportLog\.Duration()/L.get("Duration")/g' "$FILE"
sed -i 's/L\.ExportLog\.NoEntriesMessage()/L.get("No export log entries found. Export some activities to YouTrack to see them logged here.")/g' "$FILE"
sed -i 's/L\.ExportLog\.PreviousPage()/L.get("< Previous")/g' "$FILE"
sed -i 's/L\.ExportLog\.NextPage()/L.get("Next >")/g' "$FILE"
sed -i 's/L\.ExportLog\.PageIndicator()/L.get("Page %d of %d")/g' "$FILE"
sed -i 's/L\.ExportLog\.ClearLog()/L.get("Clear Log")/g' "$FILE"
sed -i 's/L\.ExportLog\.ClearLogConfirmTitle()/L.get("Clear Log Confirmation")/g' "$FILE"
sed -i 's/L\.ExportLog\.ClearLogConfirmMessage()/L.get("Are you sure you want to clear all export log entries?")/g' "$FILE"
sed -i 's/L\.ExportLog\.ClearLogWarning()/L.get("This action cannot be undone.")/g' "$FILE"
sed -i 's/L\.ExportLog\.ClearLogAction()/L.get("Yes, Clear Log")/g' "$FILE"

# KTalkImport
sed -i 's/L\.KTalkImport\.Title()/L.get("Import from KTalk")/g' "$FILE"
sed -i 's/L\.KTalkImport\.Description()/L.get("Import conference history from KTalk. Copy the fetch() request from your browser'\''s DevTools Network tab.")/g' "$FILE"
sed -i 's/L\.KTalkImport\.FromDate()/L.get("From Date")/g' "$FILE"
sed -i 's/L\.KTalkImport\.ToDate()/L.get("To Date")/g' "$FILE"
sed -i 's/L\.KTalkImport\.FetchPayload()/L.get("Fetch Payload:")/g' "$FILE"
sed -i 's/L\.KTalkImport\.SelectDate()/L.get("Select date to import:")/g' "$FILE"
sed -i 's/L\.KTalkImport\.Import()/L.get("Import")/g' "$FILE"
sed -i 's/L\.KTalkImport\.Importing()/L.get("Importing...")/g' "$FILE"
sed -i 's/L\.KTalkImport\.ImportSuccess()/L.get("Import successful")/g' "$FILE"
sed -i 's/L\.KTalkImport\.ImportSuccessMessage()/L.get("Successfully imported %d conferences.")/g' "$FILE"
sed -i 's/L\.KTalkImport\.ImportError()/L.get("Import failed")/g' "$FILE"
sed -i 's/L\.KTalkImport\.NoConferences()/L.get("No conferences found for the selected date.")/g' "$FILE"

echo "Migration complete! Backup saved as $FILE.bak"
echo "Please review the changes and test the application."
