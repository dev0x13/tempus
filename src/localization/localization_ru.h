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
        {"KTalk Import", "Импорт из KTalk"},
        {"Settings", "Настройки"},

        // Date Selector
        {"From", "С"},
        {"to", "по"},
        {"Today", "Сегодня"},
        {"This Week", "Эта неделя"},
        {"This Month", "Этот месяц"},

        // Time Entries
        {"Activity", "Активность"},
        {"Description", "Описание"},
        {"Start", "Начало"},
        {"End", "Конец"},
        {"Duration", "Длительность"},
        {"Actions", "Действия"},
        {"Edit Activity", "Изменить активность"},
        {"Add Activity", "Добавить активность"},
        {"Delete Activity", "Удалить активность"},
        {"No entries for selected date range", "Нет записей за выбранный период"},
        {"Currently tracking", "Отслеживается сейчас"},
        {"Total", "Всего"},
        {"Show activity breakdown", "Показать разбивку по активностям"},
        {"Hide activity breakdown", "Скрыть разбивку по активностям"},
        {"Activity name", "Название активности"},
        {"Optional description", "Описание (необязательно)"},
        {"(ongoing)", "(в процессе)"},
        {"Edit Entry", "Редактировать запись"},
        {"Add Entry", "Добавить запись"},
        {"Activity:", "Активность:"},
        {"Description:", "Описание:"},
        {"Start:", "Начало:"},
        {"End:", "Конец:"},
        {"Ongoing", "В процессе"},

        // Settings
        {"YouTrack Settings", "Настройки YouTrack"},
        {"URL:", "URL:"},
        {"Token:", "Токен:"},
        {"YouTrack server URL (e.g., https://youtrack.company.com)", "URL сервера YouTrack (например, https://youtrack.company.com)"},
        {"YouTrack permanent token for authentication", "Постоянный токен YouTrack для аутентификации"},
        {"Export Log", "Журнал экспорта"},
        {"Activity Aliases", "Псевдонимы активностей"},
        {"Map activity names to YouTrack issue IDs:", "Сопоставьте названия активностей с ID задач YouTrack:"},
        {"Activity Name", "Название активности"},
        {"Issue ID", "ID задачи"},
        {"+ Add Alias", "+ Добавить псевдоним"},
        {"KTalk Import", "Импорт из KTalk"},
        {"Include unplanned meetings", "Включить незапланированные встречи"},
        {"When enabled, imports all conferences including those without a title.\\nWhen disabled, only imports conferences that have a title.", "Если включено, импортирует все конференции, включая те, у которых нет названия.\\nЕсли выключено, импортирует только конференции с названием."},
        {"URL must start with http:// or https://", "URL должен начинаться с http:// или https://"},
        {"All alias entries must have both activity name and issue ID", "Все записи псевдонимов должны иметь как название активности, так и ID задачи"},
        {"Language:", "Язык:"},
        {"English", "English"},
        {"Russian", "Русский"},

        // System Tray
        {"Tempus - Idle", "Tempus - Простой"},
        {"Tempus - Tracking: ", "Tempus - Отслеживание: "},
        {"Quick Add Activity", "Быстрое добавление активности"},
        {"Show Window", "Показать окно"},
        {"Stop Tracking", "Остановить отслеживание"},
        {"Stop Tracking: ", "Остановить отслеживание: "},
        {"Recent Activities", "Недавние активности"},
        {"Exit", "Выход"},
        {"Tracking: ", "Отслеживание: "},

        // Export
        {"Export to YouTrack", "Экспорт в YouTrack"},
        {"Export %d time entries to YouTrack?", "Экспортировать %d записей времени в YouTrack?"},
        {"Confirm", "Подтвердить"},
        {"Exporting...", "Экспорт..."},
        {"Exporting %d of %d entries...", "Экспорт %d из %d записей..."},
        {"Export Complete", "Экспорт завершен"},
        {"Successfully exported %d entries to YouTrack.", "Успешно экспортировано %d записей в YouTrack."},
        {"Export Error", "Ошибка экспорта"},
        {"Failed to export entries:\\n%s", "Не удалось экспортировать записи:\\n%s"},
        {"CSV exported successfully", "CSV успешно экспортирован"},
        {"Failed to export CSV", "Не удалось экспортировать CSV"},

        // Errors
        {"Time Overlap Detected", "Обнаружено пересечение времени"},
        {"This entry overlaps with an existing entry:\\n\\nExisting: %s (%s - %s)\\n\\nPlease adjust the times to avoid overlap.", "Эта запись пересекается с существующей записью:\\n\\nСуществующая: %s (%s - %s)\\n\\nПожалуйста, измените время, чтобы избежать пересечения."},
        {"End time must be after start time", "Время окончания должно быть после времени начала"},
        {"Activity name is required", "Требуется название активности"},
        {"YouTrack is not configured. Please set URL and token in Settings.", "YouTrack не настроен. Пожалуйста, установите URL и токен в настройках."},
        {"No YouTrack issue ID configured for activity: %s", "Не настроен ID задачи YouTrack для активности: %s"},
        {"Cannot export: the following activities have no YouTrack issue ID mapping:", "Невозможно экспортировать: следующие активности не имеют сопоставления с ID задачи YouTrack:"},
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
        {"Quick Add Activity", "Быстрое добавление активности"},
        {"Start Tracking", "Начать отслеживание"},

        // Export Log
        {"YouTrack Export Log", "Журнал экспорта YouTrack"},
        {"Export Time", "Время экспорта"},
        {"Issue ID", "ID задачи"},
        {"Tracked Date", "Дата отслеживания"},
        {"No export log entries found. Export some activities to YouTrack to see them logged here.", "Записи журнала экспорта не найдены. Экспортируйте некоторые активности в YouTrack, чтобы увидеть их здесь."},
        {"< Previous", "< Предыдущая"},
        {"Next >", "Следующая >"},
        {"Page %d of %d", "Страница %d из %d"},
        {"Clear Log", "Очистить журнал"},
        {"Clear Log Confirmation", "Подтверждение очистки журнала"},
        {"Are you sure you want to clear all export log entries?", "Вы уверены, что хотите очистить все записи журнала экспорта?"},
        {"This action cannot be undone.", "Это действие нельзя отменить."},
        {"Yes, Clear Log", "Да, очистить журнал"},
        {"Date", "Дата"},
        {"Status", "Статус"},
        {"Message", "Сообщение"},
        {"No export log entries", "Нет записей в журнале экспорта"},

        // KTalk Import
        {"Import from KTalk", "Импорт из KTalk"},
        {"Import conference history from KTalk. Copy the fetch() request from your browser's DevTools Network tab.", "Импорт истории конференций из KTalk. Скопируйте запрос fetch() из вкладки Network в DevTools вашего браузера."},
        {"From Date", "Дата начала"},
        {"To Date", "Дата окончания"},
        {"Fetch Payload:", "Данные запроса:"},
        {"Select date to import:", "Выберите дату для импорта:"},
        {"Import", "Импорт"},
        {"Importing...", "Импортирование..."},
        {"Import successful", "Импорт успешен"},
        {"Successfully imported %d conferences.", "Успешно импортировано конференций: %d."},
        {"Import failed", "Импорт не удался"},
        {"No conferences found for the selected date.", "Не найдено конференций для выбранной даты."},
        {"KTalk meeting", "Встреча в KTalk"}
    };
}

// Integer values (e.g., FirstDayOfWeek)
inline std::unordered_map<std::string, int> getIntValues() {
    return {
        {"FirstDayOfWeek", 1}, // Monday
    };
}

} // namespace timetracker::localization::ru
