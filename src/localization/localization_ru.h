#pragma once

namespace timetracker::localization::ru {

// Common strings
struct CommonStrings {
    static constexpr const char* Save() { return "Сохранить"; }
    static constexpr const char* Cancel() { return "Отмена"; }
    static constexpr const char* OK() { return "ОК"; }
    static constexpr const char* Error() { return "Ошибка"; }
    static constexpr const char* Success() { return "Успешно"; }
    static constexpr const char* Warning() { return "Предупреждение"; }
    static constexpr const char* Delete() { return "Удалить"; }
    static constexpr const char* Add() { return "Добавить"; }
    static constexpr const char* Edit() { return "Изменить"; }
    static constexpr const char* Close() { return "Закрыть"; }
    static constexpr const char* Yes() { return "Да"; }
    static constexpr const char* No() { return "Нет"; }
};

// Main window / Top buttons
struct MainWindowStrings {
    static constexpr const char* Title() { return "Трекер Времени"; }
    static constexpr const char* Start() { return "Старт"; }
    static constexpr const char* Stop() { return "Стоп"; }
    static constexpr const char* ExportCSV() { return "Экспорт CSV"; }
    static constexpr const char* ExportToYouTrack() { return "Экспорт в YouTrack"; }
    static constexpr const char* KTalkImport() { return "Импорт KTalk"; }
    static constexpr const char* Settings() { return "Настройки"; }
};

// Date selector
struct DateSelectorStrings {
    static constexpr const char* From() { return "С"; }
    static constexpr const char* To() { return "по"; }
    static constexpr const char* Today() { return "Сегодня"; }
    static constexpr const char* ThisWeek() { return "Эта неделя"; }
    static constexpr const char* ThisMonth() { return "Этот месяц"; }
};

// Time entries view
struct TimeEntriesStrings {
    static constexpr const char* Activity() { return "Активность"; }
    static constexpr const char* Description() { return "Описание"; }
    static constexpr const char* StartTime() { return "Начало"; }
    static constexpr const char* EndTime() { return "Конец"; }
    static constexpr const char* Duration() { return "Длительность"; }
    static constexpr const char* Actions() { return "Действия"; }
    static constexpr const char* EditActivity() { return "Изменить активность"; }
    static constexpr const char* AddActivity() { return "Добавить активность"; }
    static constexpr const char* DeleteActivity() { return "Удалить активность"; }
    static constexpr const char* NoEntries() { return "Нет записей за выбранный период"; }
    static constexpr const char* CurrentlyTracking() { return "Отслеживается сейчас"; }
    static constexpr const char* Total() { return "Всего"; }
    static constexpr const char* ShowBreakdown() { return "Показать разбивку по активностям"; }
    static constexpr const char* HideBreakdown() { return "Скрыть разбивку по активностям"; }
    static constexpr const char* ActivityNamePlaceholder() { return "Название активности"; }
    static constexpr const char* DescriptionPlaceholder() { return "Описание (необязательно)"; }
};

// Settings dialog
struct SettingsStrings {
    static constexpr const char* Title() { return "Настройки"; }
    static constexpr const char* YouTrackSettings() { return "Настройки YouTrack"; }
    static constexpr const char* URL() { return "URL:"; }
    static constexpr const char* Token() { return "Токен:"; }
    static constexpr const char* URLTooltip() { return "URL сервера YouTrack (например, https://youtrack.company.com)"; }
    static constexpr const char* TokenTooltip() { return "Постоянный токен YouTrack для аутентификации"; }
    static constexpr const char* ExportLog() { return "Журнал экспорта"; }
    static constexpr const char* ActivityAliases() { return "Псевдонимы активностей"; }
    static constexpr const char* MapActivityNames() { return "Сопоставление названий активностей с ID задач YouTrack:"; }
    static constexpr const char* ActivityName() { return "Название активности"; }
    static constexpr const char* IssueID() { return "ID задачи"; }
    static constexpr const char* AddAlias() { return "+ Добавить псевдоним"; }
    static constexpr const char* KTalkImportSettings() { return "Импорт KTalk"; }
    static constexpr const char* IncludeUnplannedMeetings() { return "Включать незапланированные встречи"; }
    static constexpr const char* IncludeUnplannedTooltip() { return "Если включено, импортируются все конференции, включая те, у которых нет названия.\nЕсли выключено, импортируются только конференции с названием."; }
    static constexpr const char* URLValidationError() { return "URL должен начинаться с http:// или https://"; }
    static constexpr const char* AliasValidationError() { return "Все записи псевдонимов должны содержать и название активности, и ID задачи"; }
    static constexpr const char* Language() { return "Язык:"; }
    static constexpr const char* LanguageEnglish() { return "Английский"; }
    static constexpr const char* LanguageRussian() { return "Русский"; }
};

// System tray
struct SystemTrayStrings {
    static constexpr const char* Idle() { return "Трекер Времени - Простой"; }
    static constexpr const char* TrackingPrefix() { return "Трекер Времени - Отслеживание: "; }
    static constexpr const char* QuickAddActivity() { return "Быстрое добавление"; }
    static constexpr const char* ShowWindow() { return "Показать окно"; }
    static constexpr const char* StopTracking() { return "Остановить"; }
    static constexpr const char* StopTrackingPrefix() { return "Остановить: "; }
    static constexpr const char* RecentActivities() { return "Недавние активности"; }
    static constexpr const char* Exit() { return "Выход"; }
    static constexpr const char* Tracking() { return "Отслеживание: "; }
};

// Export dialogs
struct ExportStrings {
    static constexpr const char* ExportConfirmationTitle() { return "Экспорт в YouTrack"; }
    static constexpr const char* ExportConfirmationMessage() { return "Экспортировать %d записей в YouTrack?"; }
    static constexpr const char* Confirm() { return "Подтвердить"; }
    static constexpr const char* ExportProgress() { return "Экспорт..."; }
    static constexpr const char* ExportProgressMessage() { return "Экспорт %d из %d записей..."; }
    static constexpr const char* ExportSuccess() { return "Экспорт завершен"; }
    static constexpr const char* ExportSuccessMessage() { return "Успешно экспортировано %d записей в YouTrack."; }
    static constexpr const char* ExportError() { return "Ошибка экспорта"; }
    static constexpr const char* ExportErrorMessage() { return "Не удалось экспортировать записи:\n%s"; }
    static constexpr const char* CSVExportSuccess() { return "CSV успешно экспортирован"; }
    static constexpr const char* CSVExportError() { return "Не удалось экспортировать CSV"; }
};

// Error messages
struct ErrorStrings {
    static constexpr const char* OverlapTitle() { return "Обнаружено наложение времени"; }
    static constexpr const char* OverlapMessage() { return "Эта запись накладывается на существующую:\n\nСуществующая: %s (%s - %s)\n\nПожалуйста, измените время, чтобы избежать наложения."; }
    static constexpr const char* InvalidTimeRange() { return "Время окончания должно быть позже времени начала"; }
    static constexpr const char* ActivityNameRequired() { return "Название активности обязательно"; }
    static constexpr const char* YouTrackNotConfigured() { return "YouTrack не настроен. Пожалуйста, укажите URL и токен в настройках."; }
    static constexpr const char* NoActivityAlias() { return "Не настроен ID задачи YouTrack для активности: %s"; }
};

// Date and time formats
struct DateTimeFormats {
    static constexpr const char* DateFormat() { return "%d.%m.%Y"; }
    static constexpr const char* TimeFormat() { return "%H:%M"; }
    static constexpr const char* DateTimeFormat() { return "%d.%m.%Y %H:%M"; }
    static constexpr const char* DurationFormat() { return "%dч %02dм"; }
    static constexpr const char* MonthNames() { return "Январь,Февраль,Март,Апрель,Май,Июнь,Июль,Август,Сентябрь,Октябрь,Ноябрь,Декабрь"; }
    static constexpr const char* WeekdayNames() { return "Вс,Пн,Вт,Ср,Чт,Пт,Сб"; }
    static constexpr int FirstDayOfWeek() { return 1; } // Monday
};

// Quick Add Dialog
struct QuickAddStrings {
    static constexpr const char* Title() { return "Быстрое добавление активности"; }
    static constexpr const char* ActivityName() { return "Активность:"; }
    static constexpr const char* Description() { return "Описание:"; }
    static constexpr const char* StartTracking() { return "Начать отслеживание"; }
};

// Export Log Window
struct ExportLogStrings {
    static constexpr const char* Title() { return "Журнал экспорта YouTrack"; }
    static constexpr const char* Date() { return "Дата"; }
    static constexpr const char* Status() { return "Статус"; }
    static constexpr const char* Message() { return "Сообщение"; }
    static constexpr const char* NoEntries() { return "Нет записей в журнале экспорта"; }
    static constexpr const char* StatusSuccess() { return "Успех"; }
    static constexpr const char* StatusError() { return "Ошибка"; }
};

// KTalk Import Window
struct KTalkImportStrings {
    static constexpr const char* Title() { return "Импорт из KTalk"; }
    static constexpr const char* SelectDate() { return "Выберите дату для импорта:"; }
    static constexpr const char* Import() { return "Импорт"; }
    static constexpr const char* Importing() { return "Импортирование..."; }
    static constexpr const char* ImportSuccess() { return "Импорт успешен"; }
    static constexpr const char* ImportSuccessMessage() { return "Успешно импортировано конференций: %d."; }
    static constexpr const char* ImportError() { return "Импорт не удался"; }
    static constexpr const char* NoConferences() { return "Не найдено конференций для выбранной даты."; }
};

} // namespace timetracker::localization::ru
