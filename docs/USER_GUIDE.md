# Tempus User Guide

Tempus is a desktop application for tracking time spent on various activities. It features system tray integration, YouTrack synchronization, and KTalk meeting import.

## Table of Contents

- [Getting Started](#getting-started)
- [Main Window](#main-window)
- [Time Tracking](#time-tracking)
- [Managing Entries](#managing-entries)
- [Viewing Statistics](#viewing-statistics)
- [Exporting Data](#exporting-data)
- [YouTrack Integration](#youtrack-integration)
- [KTalk Import](#ktalk-import)
- [Settings](#settings)
- [System Tray](#system-tray)

---

## Getting Started

When you launch Tempus, the main window appears with your time entries for the current date range. The application also places an icon in your system tray for quick access.

### Data Storage

Your data is stored locally in a SQLite database:
- **Windows**: `%APPDATA%/tempus/`
- **Linux**: `~/.local/share/tempus/`
- **macOS**: `~/Library/Application Support/tempus/`

---

## Main Window

The main window consists of several areas:

### Top Button Bar

| Button | Description |
|--------|-------------|
| **Start** | Begin tracking a new activity |
| **Stop** | End the current tracking session |
| **Export CSV** | Save entries to a CSV file |
| **Export to YouTrack** | Send entries to YouTrack (requires configuration) |
| **KTalk Import** | Import time entries from KTalk meetings |
| **Settings** | Open application settings |

### Date Range Selector

Select which entries to display using:
- **From/To date pickers**: Click to open a calendar
- **Quick filters**:
  - **Today**: Show only today's entries
  - **This Week**: Show entries from Monday to today
  - **This Month**: Show entries from the 1st to today

### Entries List

Entries are grouped by date and displayed in a table with:
- Start time
- End time (or "ongoing" for active entries)
- Activity name and description
- Duration

**Note**: Overlapping entries are highlighted in red.

### Footer

- **Total time**: Grand total for the selected date range
- Click **Total** to expand/collapse the activity breakdown, showing time spent per activity

---

## Time Tracking

### Starting a Timer

1. Click the **Start** button
2. Enter the activity name
3. Optionally add a description
4. Click **Save** or press **Enter**

The timer begins immediately and appears in the entries list as "ongoing".

### Stopping a Timer

Click the **Stop** button to end the current tracking session. The end time is recorded automatically.

### Quick Add from System Tray

Right-click the tray icon and select **Quick Add** to start tracking without opening the main window.

---

## Managing Entries

### Editing an Entry

1. Click on any entry in the list
2. The edit dialog opens with:
   - Activity name
   - Description
   - Start date and time
   - End date and time (or "Ongoing" checkbox)
3. Make your changes
4. Click **Save**

### Deleting an Entry

1. Click on the entry to open the edit dialog
2. Click **Delete**
3. Confirm the deletion

### Creating a Manual Entry

Use the Start button to create entries with specific times:
1. Click **Start**
2. Fill in activity name and description
3. Set the start date/time
4. Uncheck "Ongoing" and set the end date/time
5. Click **Save**

---

## Viewing Statistics

### Activity Breakdown

1. Select your desired date range
2. Click the **Total** row at the bottom of the window
3. The breakdown expands to show:
   - Each activity with total time spent
   - Sorted by duration (longest first)
   - Export status indicators

### Date Range Analysis

Use the date pickers or quick filters to analyze different time periods. The total and breakdown update automatically.

---

## Exporting Data

### CSV Export

1. Select the date range to export
2. Click **Export CSV**
3. Choose a save location
4. The file includes: Start Time, End Time, Activity, Duration, Description

### YouTrack Export

See [YouTrack Integration](#youtrack-integration) below.

---

## YouTrack Integration

### Initial Setup

1. Open **Settings**
2. Enter your YouTrack instance URL (e.g., `https://youtrack.company.com`)
3. Enter your API token (generate from YouTrack user settings)
4. Configure **Activity Aliases** to map activity names to issue IDs:
   - Example: "Development" -> "PROJ-123"

### Exporting to YouTrack

1. Select the date range to export
2. Ensure there are no overlapping entries (highlighted in red)
3. Click **Export to YouTrack**
4. Review the preview showing aggregated work items:
   - Time is grouped by activity and date
   - Minutes are rounded up to nearest 10 (minimum 10)
5. Click **Confirm** to export
6. Monitor the progress bar
7. View results in the success dialog

### Export Log

- Open **Settings** and click **Export Log** to view history
- The log shows previously exported items with status
- Use **Clear Log** to remove history

### Troubleshooting

- **Overlapping entries**: Resolve time conflicts before exporting
- **Missing aliases**: Add activity aliases in Settings
- **Authentication errors**: Verify your API token is valid

---

## KTalk Import

Import time entries from KTalk video conferences.

### How to Import

1. Go to KTalk web interface in your browser
2. Open Developer Tools (F12) -> Network tab
3. Search for conferences to trigger an API request
4. Find the request and copy the fetch() call (right-click -> Copy as fetch)
5. In Tempus, click **KTalk Import**
6. Paste the fetch payload into the text area
7. Adjust the date range if needed
8. Click **Import**

### Options

In **Settings**, you can toggle whether to include unplanned meetings in the import.

---

## Settings

Access settings by clicking the **Settings** button.

### Language

Switch between English and Russian. Changes take effect immediately.

### YouTrack Configuration

- **Base URL**: Your YouTrack instance address
- **Token**: API authentication token
- **Activity Aliases**: Map activity names to issue IDs

### KTalk Options

- **Include unplanned meetings**: Toggle whether to import unplanned conferences

### Export Log

View and manage the history of YouTrack exports.

---

## System Tray

The application minimizes to the system tray when you close the window.

### Tray Icon

- Different icons indicate idle vs. active tracking
- Updates every second when tracking

### Tray Menu

Right-click the tray icon for options:
- **Show Window**: Open the main window
- **Quick Add**: Start tracking a new activity quickly
- **Stop**: Stop the current timer
- **Exit**: Close the application completely

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| **Enter** | Confirm action in dialogs |
| **Escape** | Close dialogs |
| **Tab** | Navigate between form fields |

---

## Tips

1. **Use Quick Add** for rapid time entry without opening the main window
2. **Check for overlaps** (red highlighting) before exporting to YouTrack
3. **Set up activity aliases** once to streamline YouTrack exports
4. **Use quick filters** (Today, This Week, This Month) to quickly view relevant entries
5. **Expand the Total row** to see how your time is distributed across activities
