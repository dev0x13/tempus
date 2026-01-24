#pragma once

#include <string>
#include <cstdint>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace timetracker::utils {

class TimeUtils {
public:
    // Get current Unix timestamp
    static int64_t now();

    // Get start of day for a given timestamp
    static int64_t startOfDay(int64_t timestamp);

    // Get end of day for a given timestamp
    static int64_t endOfDay(int64_t timestamp);

    // Get start of week (Monday) for a given timestamp
    static int64_t startOfWeek(int64_t timestamp);

    // Get start of month for a given timestamp
    static int64_t startOfMonth(int64_t timestamp);

    // Get end of month for a given timestamp
    static int64_t endOfMonth(int64_t timestamp);

    // Convert timestamp to local time components
    static std::tm toLocalTime(int64_t timestamp);

    // Create timestamp from local time components
    static int64_t fromLocalTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);

    // Format duration as "Xh Ym" or "Ym Zs"
    static std::string formatDuration(int64_t seconds);

    // Format duration as "HH:MM:SS"
    static std::string formatDurationHMS(int64_t seconds);

    // Format timestamp as "YYYY-MM-DD HH:MM"
    static std::string formatDateTime(int64_t timestamp);

    // Format timestamp as "HH:MM"
    static std::string formatTime(int64_t timestamp);

    // Format timestamp as "YYYY-MM-DD"
    static std::string formatDate(int64_t timestamp);

    // Parse date string "YYYY-MM-DD" to timestamp (start of day)
    static int64_t parseDate(const std::string& dateStr);

    // Parse time string "HH:MM" and combine with date timestamp
    static int64_t parseTime(const std::string& timeStr, int64_t dateTimestamp);

    // Get ISO week number
    static int getISOWeek(int64_t timestamp);

    // Get days in month
    static int getDaysInMonth(int year, int month);

    // Check if year is leap year
    static bool isLeapYear(int year);
};

} // namespace timetracker::utils
