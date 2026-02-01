#include "LocalizationManager.hpp"
#include "localization_en.h"
#include "localization_ru.h"
#include <locale>
#include <cstdlib>

namespace timetracker::localization {

LocalizationManager::LocalizationManager() {
    // Detect system locale and set default language
    Language detectedLang = Language::Russian; // Default fallback

    const char* locale = std::getenv("LANG");
    if (locale != nullptr) {
        std::string localeStr(locale);
        if (localeStr.find("en") == 0) {
            detectedLang = Language::English;
        } else if (localeStr.find("ru") == 0) {
            detectedLang = Language::Russian;
        }
    }

    loadLanguage(detectedLang);
}

LocalizationManager& LocalizationManager::instance() {
    static LocalizationManager instance;
    return instance;
}

void LocalizationManager::setLanguage(Language lang) {
    if (lang != currentLanguage_) {
        loadLanguage(lang);
    }
}

Language LocalizationManager::getCurrentLanguage() const {
    return currentLanguage_;
}

void LocalizationManager::loadLanguage(Language lang) {
    currentLanguage_ = lang;

    if (lang == Language::English) {
        // Load English strings
        Common.Save = en::CommonStrings::Save;
        Common.Cancel = en::CommonStrings::Cancel;
        Common.OK = en::CommonStrings::OK;
        Common.Error = en::CommonStrings::Error;
        Common.Success = en::CommonStrings::Success;
        Common.Warning = en::CommonStrings::Warning;
        Common.Delete = en::CommonStrings::Delete;
        Common.Add = en::CommonStrings::Add;
        Common.Edit = en::CommonStrings::Edit;
        Common.Close = en::CommonStrings::Close;
        Common.Yes = en::CommonStrings::Yes;
        Common.No = en::CommonStrings::No;

        MainWindow.Title = en::MainWindowStrings::Title;
        MainWindow.Start = en::MainWindowStrings::Start;
        MainWindow.Stop = en::MainWindowStrings::Stop;
        MainWindow.ExportCSV = en::MainWindowStrings::ExportCSV;
        MainWindow.ExportToYouTrack = en::MainWindowStrings::ExportToYouTrack;
        MainWindow.KTalkImport = en::MainWindowStrings::KTalkImport;
        MainWindow.Settings = en::MainWindowStrings::Settings;

        DateSelector.From = en::DateSelectorStrings::From;
        DateSelector.To = en::DateSelectorStrings::To;
        DateSelector.Today = en::DateSelectorStrings::Today;
        DateSelector.ThisWeek = en::DateSelectorStrings::ThisWeek;
        DateSelector.ThisMonth = en::DateSelectorStrings::ThisMonth;

        TimeEntries.Activity = en::TimeEntriesStrings::Activity;
        TimeEntries.Description = en::TimeEntriesStrings::Description;
        TimeEntries.StartTime = en::TimeEntriesStrings::StartTime;
        TimeEntries.EndTime = en::TimeEntriesStrings::EndTime;
        TimeEntries.Duration = en::TimeEntriesStrings::Duration;
        TimeEntries.Actions = en::TimeEntriesStrings::Actions;
        TimeEntries.EditActivity = en::TimeEntriesStrings::EditActivity;
        TimeEntries.AddActivity = en::TimeEntriesStrings::AddActivity;
        TimeEntries.DeleteActivity = en::TimeEntriesStrings::DeleteActivity;
        TimeEntries.NoEntries = en::TimeEntriesStrings::NoEntries;
        TimeEntries.CurrentlyTracking = en::TimeEntriesStrings::CurrentlyTracking;
        TimeEntries.Total = en::TimeEntriesStrings::Total;
        TimeEntries.ShowBreakdown = en::TimeEntriesStrings::ShowBreakdown;
        TimeEntries.HideBreakdown = en::TimeEntriesStrings::HideBreakdown;
        TimeEntries.ActivityNamePlaceholder = en::TimeEntriesStrings::ActivityNamePlaceholder;
        TimeEntries.DescriptionPlaceholder = en::TimeEntriesStrings::DescriptionPlaceholder;

        Settings.Title = en::SettingsStrings::Title;
        Settings.YouTrackSettings = en::SettingsStrings::YouTrackSettings;
        Settings.URL = en::SettingsStrings::URL;
        Settings.Token = en::SettingsStrings::Token;
        Settings.URLTooltip = en::SettingsStrings::URLTooltip;
        Settings.TokenTooltip = en::SettingsStrings::TokenTooltip;
        Settings.ExportLog = en::SettingsStrings::ExportLog;
        Settings.ActivityAliases = en::SettingsStrings::ActivityAliases;
        Settings.MapActivityNames = en::SettingsStrings::MapActivityNames;
        Settings.ActivityName = en::SettingsStrings::ActivityName;
        Settings.IssueID = en::SettingsStrings::IssueID;
        Settings.AddAlias = en::SettingsStrings::AddAlias;
        Settings.KTalkImportSettings = en::SettingsStrings::KTalkImportSettings;
        Settings.IncludeUnplannedMeetings = en::SettingsStrings::IncludeUnplannedMeetings;
        Settings.IncludeUnplannedTooltip = en::SettingsStrings::IncludeUnplannedTooltip;
        Settings.URLValidationError = en::SettingsStrings::URLValidationError;
        Settings.AliasValidationError = en::SettingsStrings::AliasValidationError;
        Settings.Language = en::SettingsStrings::Language;
        Settings.LanguageEnglish = en::SettingsStrings::LanguageEnglish;
        Settings.LanguageRussian = en::SettingsStrings::LanguageRussian;

        SystemTray.Idle = en::SystemTrayStrings::Idle;
        SystemTray.TrackingPrefix = en::SystemTrayStrings::TrackingPrefix;
        SystemTray.QuickAddActivity = en::SystemTrayStrings::QuickAddActivity;
        SystemTray.ShowWindow = en::SystemTrayStrings::ShowWindow;
        SystemTray.StopTracking = en::SystemTrayStrings::StopTracking;
        SystemTray.StopTrackingPrefix = en::SystemTrayStrings::StopTrackingPrefix;
        SystemTray.RecentActivities = en::SystemTrayStrings::RecentActivities;
        SystemTray.Exit = en::SystemTrayStrings::Exit;
        SystemTray.Tracking = en::SystemTrayStrings::Tracking;

        Export.ExportConfirmationTitle = en::ExportStrings::ExportConfirmationTitle;
        Export.ExportConfirmationMessage = en::ExportStrings::ExportConfirmationMessage;
        Export.Confirm = en::ExportStrings::Confirm;
        Export.ExportProgress = en::ExportStrings::ExportProgress;
        Export.ExportProgressMessage = en::ExportStrings::ExportProgressMessage;
        Export.ExportSuccess = en::ExportStrings::ExportSuccess;
        Export.ExportSuccessMessage = en::ExportStrings::ExportSuccessMessage;
        Export.ExportError = en::ExportStrings::ExportError;
        Export.ExportErrorMessage = en::ExportStrings::ExportErrorMessage;
        Export.CSVExportSuccess = en::ExportStrings::CSVExportSuccess;
        Export.CSVExportError = en::ExportStrings::CSVExportError;

        Errors.OverlapTitle = en::ErrorStrings::OverlapTitle;
        Errors.OverlapMessage = en::ErrorStrings::OverlapMessage;
        Errors.InvalidTimeRange = en::ErrorStrings::InvalidTimeRange;
        Errors.ActivityNameRequired = en::ErrorStrings::ActivityNameRequired;
        Errors.YouTrackNotConfigured = en::ErrorStrings::YouTrackNotConfigured;
        Errors.NoActivityAlias = en::ErrorStrings::NoActivityAlias;

        DateTime.DateFormat = en::DateTimeFormats::DateFormat;
        DateTime.TimeFormat = en::DateTimeFormats::TimeFormat;
        DateTime.DateTimeFormat = en::DateTimeFormats::DateTimeFormat;
        DateTime.DurationFormat = en::DateTimeFormats::DurationFormat;
        DateTime.MonthNames = en::DateTimeFormats::MonthNames;
        DateTime.WeekdayNames = en::DateTimeFormats::WeekdayNames;
        DateTime.FirstDayOfWeek = en::DateTimeFormats::FirstDayOfWeek;

        QuickAdd.Title = en::QuickAddStrings::Title;
        QuickAdd.ActivityName = en::QuickAddStrings::ActivityName;
        QuickAdd.Description = en::QuickAddStrings::Description;
        QuickAdd.StartTracking = en::QuickAddStrings::StartTracking;

        ExportLog.Title = en::ExportLogStrings::Title;
        ExportLog.Date = en::ExportLogStrings::Date;
        ExportLog.Status = en::ExportLogStrings::Status;
        ExportLog.Message = en::ExportLogStrings::Message;
        ExportLog.NoEntries = en::ExportLogStrings::NoEntries;
        ExportLog.StatusSuccess = en::ExportLogStrings::StatusSuccess;
        ExportLog.StatusError = en::ExportLogStrings::StatusError;

        KTalkImport.Title = en::KTalkImportStrings::Title;
        KTalkImport.SelectDate = en::KTalkImportStrings::SelectDate;
        KTalkImport.Import = en::KTalkImportStrings::Import;
        KTalkImport.Importing = en::KTalkImportStrings::Importing;
        KTalkImport.ImportSuccess = en::KTalkImportStrings::ImportSuccess;
        KTalkImport.ImportSuccessMessage = en::KTalkImportStrings::ImportSuccessMessage;
        KTalkImport.ImportError = en::KTalkImportStrings::ImportError;
        KTalkImport.NoConferences = en::KTalkImportStrings::NoConferences;
    } else {
        // Load Russian strings
        Common.Save = ru::CommonStrings::Save;
        Common.Cancel = ru::CommonStrings::Cancel;
        Common.OK = ru::CommonStrings::OK;
        Common.Error = ru::CommonStrings::Error;
        Common.Success = ru::CommonStrings::Success;
        Common.Warning = ru::CommonStrings::Warning;
        Common.Delete = ru::CommonStrings::Delete;
        Common.Add = ru::CommonStrings::Add;
        Common.Edit = ru::CommonStrings::Edit;
        Common.Close = ru::CommonStrings::Close;
        Common.Yes = ru::CommonStrings::Yes;
        Common.No = ru::CommonStrings::No;

        MainWindow.Title = ru::MainWindowStrings::Title;
        MainWindow.Start = ru::MainWindowStrings::Start;
        MainWindow.Stop = ru::MainWindowStrings::Stop;
        MainWindow.ExportCSV = ru::MainWindowStrings::ExportCSV;
        MainWindow.ExportToYouTrack = ru::MainWindowStrings::ExportToYouTrack;
        MainWindow.KTalkImport = ru::MainWindowStrings::KTalkImport;
        MainWindow.Settings = ru::MainWindowStrings::Settings;

        DateSelector.From = ru::DateSelectorStrings::From;
        DateSelector.To = ru::DateSelectorStrings::To;
        DateSelector.Today = ru::DateSelectorStrings::Today;
        DateSelector.ThisWeek = ru::DateSelectorStrings::ThisWeek;
        DateSelector.ThisMonth = ru::DateSelectorStrings::ThisMonth;

        TimeEntries.Activity = ru::TimeEntriesStrings::Activity;
        TimeEntries.Description = ru::TimeEntriesStrings::Description;
        TimeEntries.StartTime = ru::TimeEntriesStrings::StartTime;
        TimeEntries.EndTime = ru::TimeEntriesStrings::EndTime;
        TimeEntries.Duration = ru::TimeEntriesStrings::Duration;
        TimeEntries.Actions = ru::TimeEntriesStrings::Actions;
        TimeEntries.EditActivity = ru::TimeEntriesStrings::EditActivity;
        TimeEntries.AddActivity = ru::TimeEntriesStrings::AddActivity;
        TimeEntries.DeleteActivity = ru::TimeEntriesStrings::DeleteActivity;
        TimeEntries.NoEntries = ru::TimeEntriesStrings::NoEntries;
        TimeEntries.CurrentlyTracking = ru::TimeEntriesStrings::CurrentlyTracking;
        TimeEntries.Total = ru::TimeEntriesStrings::Total;
        TimeEntries.ShowBreakdown = ru::TimeEntriesStrings::ShowBreakdown;
        TimeEntries.HideBreakdown = ru::TimeEntriesStrings::HideBreakdown;
        TimeEntries.ActivityNamePlaceholder = ru::TimeEntriesStrings::ActivityNamePlaceholder;
        TimeEntries.DescriptionPlaceholder = ru::TimeEntriesStrings::DescriptionPlaceholder;

        Settings.Title = ru::SettingsStrings::Title;
        Settings.YouTrackSettings = ru::SettingsStrings::YouTrackSettings;
        Settings.URL = ru::SettingsStrings::URL;
        Settings.Token = ru::SettingsStrings::Token;
        Settings.URLTooltip = ru::SettingsStrings::URLTooltip;
        Settings.TokenTooltip = ru::SettingsStrings::TokenTooltip;
        Settings.ExportLog = ru::SettingsStrings::ExportLog;
        Settings.ActivityAliases = ru::SettingsStrings::ActivityAliases;
        Settings.MapActivityNames = ru::SettingsStrings::MapActivityNames;
        Settings.ActivityName = ru::SettingsStrings::ActivityName;
        Settings.IssueID = ru::SettingsStrings::IssueID;
        Settings.AddAlias = ru::SettingsStrings::AddAlias;
        Settings.KTalkImportSettings = ru::SettingsStrings::KTalkImportSettings;
        Settings.IncludeUnplannedMeetings = ru::SettingsStrings::IncludeUnplannedMeetings;
        Settings.IncludeUnplannedTooltip = ru::SettingsStrings::IncludeUnplannedTooltip;
        Settings.URLValidationError = ru::SettingsStrings::URLValidationError;
        Settings.AliasValidationError = ru::SettingsStrings::AliasValidationError;
        Settings.Language = ru::SettingsStrings::Language;
        Settings.LanguageEnglish = ru::SettingsStrings::LanguageEnglish;
        Settings.LanguageRussian = ru::SettingsStrings::LanguageRussian;

        SystemTray.Idle = ru::SystemTrayStrings::Idle;
        SystemTray.TrackingPrefix = ru::SystemTrayStrings::TrackingPrefix;
        SystemTray.QuickAddActivity = ru::SystemTrayStrings::QuickAddActivity;
        SystemTray.ShowWindow = ru::SystemTrayStrings::ShowWindow;
        SystemTray.StopTracking = ru::SystemTrayStrings::StopTracking;
        SystemTray.StopTrackingPrefix = ru::SystemTrayStrings::StopTrackingPrefix;
        SystemTray.RecentActivities = ru::SystemTrayStrings::RecentActivities;
        SystemTray.Exit = ru::SystemTrayStrings::Exit;
        SystemTray.Tracking = ru::SystemTrayStrings::Tracking;

        Export.ExportConfirmationTitle = ru::ExportStrings::ExportConfirmationTitle;
        Export.ExportConfirmationMessage = ru::ExportStrings::ExportConfirmationMessage;
        Export.Confirm = ru::ExportStrings::Confirm;
        Export.ExportProgress = ru::ExportStrings::ExportProgress;
        Export.ExportProgressMessage = ru::ExportStrings::ExportProgressMessage;
        Export.ExportSuccess = ru::ExportStrings::ExportSuccess;
        Export.ExportSuccessMessage = ru::ExportStrings::ExportSuccessMessage;
        Export.ExportError = ru::ExportStrings::ExportError;
        Export.ExportErrorMessage = ru::ExportStrings::ExportErrorMessage;
        Export.CSVExportSuccess = ru::ExportStrings::CSVExportSuccess;
        Export.CSVExportError = ru::ExportStrings::CSVExportError;

        Errors.OverlapTitle = ru::ErrorStrings::OverlapTitle;
        Errors.OverlapMessage = ru::ErrorStrings::OverlapMessage;
        Errors.InvalidTimeRange = ru::ErrorStrings::InvalidTimeRange;
        Errors.ActivityNameRequired = ru::ErrorStrings::ActivityNameRequired;
        Errors.YouTrackNotConfigured = ru::ErrorStrings::YouTrackNotConfigured;
        Errors.NoActivityAlias = ru::ErrorStrings::NoActivityAlias;

        DateTime.DateFormat = ru::DateTimeFormats::DateFormat;
        DateTime.TimeFormat = ru::DateTimeFormats::TimeFormat;
        DateTime.DateTimeFormat = ru::DateTimeFormats::DateTimeFormat;
        DateTime.DurationFormat = ru::DateTimeFormats::DurationFormat;
        DateTime.MonthNames = ru::DateTimeFormats::MonthNames;
        DateTime.WeekdayNames = ru::DateTimeFormats::WeekdayNames;
        DateTime.FirstDayOfWeek = ru::DateTimeFormats::FirstDayOfWeek;

        QuickAdd.Title = ru::QuickAddStrings::Title;
        QuickAdd.ActivityName = ru::QuickAddStrings::ActivityName;
        QuickAdd.Description = ru::QuickAddStrings::Description;
        QuickAdd.StartTracking = ru::QuickAddStrings::StartTracking;

        ExportLog.Title = ru::ExportLogStrings::Title;
        ExportLog.Date = ru::ExportLogStrings::Date;
        ExportLog.Status = ru::ExportLogStrings::Status;
        ExportLog.Message = ru::ExportLogStrings::Message;
        ExportLog.NoEntries = ru::ExportLogStrings::NoEntries;
        ExportLog.StatusSuccess = ru::ExportLogStrings::StatusSuccess;
        ExportLog.StatusError = ru::ExportLogStrings::StatusError;

        KTalkImport.Title = ru::KTalkImportStrings::Title;
        KTalkImport.SelectDate = ru::KTalkImportStrings::SelectDate;
        KTalkImport.Import = ru::KTalkImportStrings::Import;
        KTalkImport.Importing = ru::KTalkImportStrings::Importing;
        KTalkImport.ImportSuccess = ru::KTalkImportStrings::ImportSuccess;
        KTalkImport.ImportSuccessMessage = ru::KTalkImportStrings::ImportSuccessMessage;
        KTalkImport.ImportError = ru::KTalkImportStrings::ImportError;
        KTalkImport.NoConferences = ru::KTalkImportStrings::NoConferences;
    }
}

} // namespace timetracker::localization
