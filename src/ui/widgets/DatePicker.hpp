#pragma once

#include "utils/TimeUtils.hpp"
#include <cstdint>
#include <string>

namespace timetracker::ui::widgets {

class DatePicker {
public:
    // Render a date picker widget
    // Returns true if the date was changed
    // date: array of 3 ints [year, month, day]
    static bool render(const char* label, int* date);

    // Render a date picker with popup calendar
    static bool renderWithCalendar(const char* label, int* date);

    // Convert timestamp to date array
    static void timestampToDate(int64_t timestamp, int* date);

    // Convert date array to timestamp (start of day)
    static int64_t dateToTimestamp(const int* date);

private:
    static void renderCalendarPopup(const char* popupId, int* date, bool& changed);
    static int getDaysInMonth(int year, int month);
    static int getFirstDayOfMonth(int year, int month);
};

} // namespace timetracker::ui::widgets
