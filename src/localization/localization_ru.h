#pragma once

#include <unordered_map>
#include <string>

namespace timetracker::localization::ru {

// Russian translations map: English key -> Russian translation
inline std::unordered_map<std::string, std::string> getTranslations() {
    return {
        // Common
        {"Save", "Сохранить"},
        {"Cancel", "Отмена"},
        {"OK", "ОК"},
        {"Error", "Ошибка"},
        {"Success", "Успех"},
        {"Warning", "Предупреждение"},
        {"Delete", "Удалить"},
        {"Add", "Добавить"},
        {"Edit", "Изменить"},
        {"Close", "Закрыть"},
        {"Yes", "Да"},
        {"No", "Нет"},

        // Main Window
        {"Tempus", "Tempus"},
        {"Start", "Старт"},
        {"Stop", "Стоп"},
        {"Export CSV", "Экспорт CSV"},
        {"Export to YouTrack", "Экспорт в YouTrack"},
        {"KTalk import", "Импорт из Толка"},
        {"Settings", "Настройки"},

        // Date Selector
        {"From", "С"},
        {"to", "по"},
        {"Today", "Сегодня"},
        {"This week", "Эта неделя"},
        {"Last 10 days", "Последние 10 дней"},

        // Time Entries
        {"Activity", "Активность"},
        {"Description", "Описание"},
        {"Start", "Начало"},
        {"End", "Конец"},
        {"Duration", "Длительность"},
        {"Actions", "Действия"},
        {"No entries for selected date range", "Нет записей за выбранный период"},
        {"Currently tracking", "Отслеживается сейчас"},
        {"Total", "Всего"},
        {"Overtime", "Переработка"},
        {"Undertime", "Недоработка"},
        {"Activity name", "Название активности"},
        {"Optional description", "Описание (необязательно)"},
        {"(ongoing)", "(в процессе)"},
        {"Edit entry", "Редактировать запись"},
        {"Add entry", "Добавить запись"},
        {"Activity:", "Активность:"},
        {"Description:", "Описание:"},
        {"Start:", "Начало:"},
        {"End:", "Конец:"},
        {"Ongoing", "В процессе"},

        // Settings
        {"YouTrack settings", "Настройки YouTrack"},
        {"URL:", "URL:"},
        {"Token:", "Токен:"},
        {"YouTrack server URL (e.g., https://youtrack.company.com)", "URL сервера YouTrack (например, https://youtrack.company.com)"},
        {"YouTrack permanent token for authentication", "Постоянный токен YouTrack для аутентификации"},
        {"Export log", "Журнал экспорта"},
        {"Activity aliases", "Псевдонимы активностей"},
        {"Arbitrary activity names can be mapped to YouTrack issue IDs for export.\nActivities named like YouTrack issue IDs (XXX-123) are exported automatically.", "Произвольные названия активностей можно сопоставить с ID задач YouTrack для экспорта.\nАктивности, названные как ID задач YouTrack (XXX-123), экспортируются автоматически."},
        {"Activity name", "Название активности"},
        {"Issue ID", "ID задачи"},
        {"+ Add Alias", "+ Добавить псевдоним"},
        {"KTalk Import", "Импорт из Толка"},
        {"Include unplanned meetings", "Включить незапланированные встречи"},
        {"When enabled, imports all meetings including those without a title.\nWhen disabled, only imports meetings that have a title.", "Если включено, импортирует все встречи, включая те, у которых нет названия.\nЕсли выключено, импортирует только встречи с названием."},
        {"Snap interval (minutes):", "Интервал выравнивания (минуты):"},
        {"Snaps meeting times to the nearest selected boundary.\n0 = disabled.", "Выравнивает время встреч по ближайшему выбранному интервалу выравнивания.\n0 - отключено."},
        {"URL must start with http:// or https://", "URL должен начинаться с http:// или https://"},
        {"All alias entries must have both activity name and issue ID", "Все записи псевдонимов должны иметь как название активности, так и ID задачи"},
        {"Language:", "Язык:"},
        {"English", "English"},
        {"Russian", "Русский"},

        // System Tray
        {"Tempus - Idle", "Tempus - Простой"},
        {"Tempus - Tracking: ", "Tempus - Отслеживание: "},
        {"Quick start", "Быстрый старт"},
        {"Show window", "Показать окно"},
        {"Today: ", "Сегодня: "},
        {"Stop: ", "Стоп: "},
        {"Continue: ", "Продолжить: "},
        {"Recent activities", "Недавние активности"},
        {"Exit", "Выход"},
        {"Tracking: ", "В процессе: "},

        // Export
        {"Export to YouTrack", "Экспорт в YouTrack"},
        {"Confirm YouTrack Export", "Подтвердите экспорт в YouTrack"},
        {"The following time entries will be exported to YouTrack:", "Следующие записи будут экспортированы в YouTrack:"},
        {"%s: %d min (%s)", "%s: %d мин (%s)"},
        {"(activity: %s)", "(активность: %s)"},
        {"Total: %s", "Всего: %s"},
        {"Export", "Экспорт"},
        {"Exporting to YouTrack", "Экспорт в YouTrack..."},
        {"Exporting activity %d of %d", "Экспорт активности %d из %d"},
        {"Export Successful", "Экспорт выполнен"},
        {"Successfully exported %s to YouTrack!", "Успешно экспортировано %s в YouTrack!"},
        {"(%d tasks)", "(%d задач)"},
        {"Export failed", "Ошибка экспорта"},
        {"Overlapping activities detected.", "Обнаружены перекрывающиеся записи."},
        {"Please fix these entries manually before exporting:", "Пожалуйста, исправьте эти записи вручную перед экспортом:"},
        {"No completed time entries found in selected date range.", "Нет завершённых записей за выбранный период."},
        {"Export cancelled. %d of %d activities exported.", "Экспорт отменён. Экспортировано %d из %d активностей."},
        {"Export %d time entries to YouTrack?", "Экспортировать %d записей времени в YouTrack?"},
        {"Confirm", "Подтвердить"},
        {"Exporting...", "Экспорт..."},
        {"Exporting %d of %d entries...", "Экспорт %d из %d записей..."},
        {"Export complete", "Экспорт завершен"},
        {"Successfully exported %d entries to YouTrack.", "Успешно экспортировано %d записей в YouTrack."},
        {"Export error", "Ошибка экспорта"},
        {"Failed to export entries:\n%s", "Не удалось экспортировать записи:\n%s"},
        {"CSV exported successfully", "CSV успешно экспортирован"},
        {"Failed to export CSV", "Не удалось экспортировать CSV"},

        // Errors
        {"Time overlap detected", "Обнаружено пересечение времени"},
        {"End time must be after start time", "Время окончания должно быть после времени начала"},
        {"Start time cannot be in the future", "Время начала не может быть в будущем"},
        {"Start time must be before end time", "Время начала должно быть раньше времени окончания"},
        {"Activity name is required", "Требуется название активности"},
        {"YouTrack is not configured. Please set URL and token in Settings.", "YouTrack не настроен. Пожалуйста, установите URL и токен в настройках."},
        {"No YouTrack issue ID configured for activity: %s", "Не настроен ID задачи YouTrack для активности: %s"},
        {"The following activities have no YouTrack issue ID mapping:", "Следующие активности не имеют сопоставления с ID задачи YouTrack:"},
        {"Add aliases for these activities in Settings.", "Добавьте псевдонимы для этих активностей в настройках."},

        // Date/Time Formats
        {"%m/%d/%Y", "%d.%m.%Y"},
        {"%I:%M %p", "%H:%M"},
        {"%m/%d/%Y %I:%M %p", "%d.%m.%Y %H:%M"},
        {"%dh %02dm", "%dч %02dм"},
        {"h", "ч"},
        {"min", "мин"},
        {"January,February,March,April,May,June,July,August,September,October,November,December", "Январь,Февраль,Март,Апрель,Май,Июнь,Июль,Август,Сентябрь,Октябрь,Ноябрь,Декабрь"},
        {"Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec", "Янв,Фев,Мар,Апр,Май,Июн,Июл,Авг,Сен,Окт,Ноя,Дек"},
        {"Sun,Mon,Tue,Wed,Thu,Fri,Sat", "Вс,Пн,Вт,Ср,Чт,Пт,Сб"},
        {"Sunday,Monday,Tuesday,Wednesday,Thursday,Friday,Saturday", "Воскресенье,Понедельник,Вторник,Среда,Четверг,Пятница,Суббота"},

        // Quick Add
        {"Quick start", "Быстрый старт"},
        {"Start tracking", "Начать"},
        {"Activity name or YouTrack issue ID", "Название активности или ID задачи в YouTrack"},

        // Export Log
        {"YouTrack export log", "Журнал экспорта YouTrack"},
        {"Export time", "Время экспорта"},
        {"Issue ID", "ID задачи"},
        {"Tracked date", "Дата отслеживания"},
        {"No export log entries found. Export some activities to YouTrack to see them logged here.", "Записи журнала экспорта не найдены. Экспортируйте некоторые активности в YouTrack, чтобы увидеть их здесь."},
        {"< Previous", "< Предыдущая"},
        {"Next >", "Следующая >"},
        {"Page %d of %d", "Страница %d из %d"},
        {"Clear log", "Очистить журнал"},
        {"Clear log confirmation", "Подтверждение очистки журнала"},
        {"Are you sure you want to clear all export log entries?", "Вы уверены, что хотите очистить все записи журнала экспорта?"},
        {"This action cannot be undone.", "Это действие нельзя отменить."},
        {"Yes, clear log", "Да, очистить журнал"},
        {"Date", "Дата"},
        {"Status", "Статус"},
        {"Message", "Сообщение"},
        {"No export log entries", "Нет записей в журнале экспорта"},

        // About
        {"About", "О программе"},
        {"Version:", "Версия:"},
        {"Build date:", "Дата сборки:"},
        {"Project homepage:", "Домашняя страница:"},

        // Block Schedule
        {"Block schedule", "Блочное расписание"},
        {"Enable block schedule view", "Включить блочное расписание"},
        {"Display your workday as a grid of time blocks that you can click to fill in", "Отображать рабочий день в виде сетки временных блоков, которые можно заполнять нажатием"},
        {"Workday start:", "Начало рабочего дня:"},
        {"Workday end:", "Конец рабочего дня:"},
        {"Outside workday", "Вне рабочего дня"},

        // KTalk Import
        {"Import from KTalk", "Импорт из Толка"},
        {"Copy the fetch() request from your browser's DevTools Network tab", "Скопируйте запрос fetch() из вкладки Network в DevTools вашего браузера"},
        {"From date", "Дата начала"},
        {"To date", "Дата окончания"},
        {"Fetch payload:", "Данные запроса:"},
        {"Select date to import:", "Выберите дату для импорта:"},
        {"Entry to edit:", "Запись для редактирования:"},
        {"Import", "Импорт"},
        {"Importing...", "Импортирование..."},
        {"Import successful", "Импорт успешен"},
        {"Successfully imported %d meetings", "Успешно импортировано встреч: %d"},
        {"Imported %d meetings, %d already existed", "Импортировано встреч: %d, уже существовало: %d"},
        {"All meetings in this range have already been imported", "Все встречи в выбранные даты уже были импортированы"},
        {"Import failed", "Импорт не удался"},
        {"No meetings found in selected date range", "Не найдено встреч в выбранные даты"},
        {"KTalk meeting", "Встреча в Толке"},
        {"Date range exceeds maximum of %d days", "Диапазон дат превышает максимум в %d дней"},
        {"End date must be after start date", "Дата окончания должна быть больше либо равна дате начала"}
    };
}

// Integer values (e.g., FirstDayOfWeek)
inline std::unordered_map<std::string, int> getIntValues() {
    return {
        {"FirstDayOfWeek", 1}, // Monday
    };
}

} // namespace timetracker::localization::ru
