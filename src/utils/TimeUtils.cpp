#include "TimeUtils.hpp"
#include "localization/LocalizationManager.hpp"
#include <stdexcept>

namespace timetracker::utils {

int64_t TimeUtils::now() {
    return static_cast<int64_t>(std::time(nullptr));
}

int64_t TimeUtils::startOfDay(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return std::mktime(&tm);
}

int64_t TimeUtils::endOfDay(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    return std::mktime(&tm);
}

int64_t TimeUtils::startOfWeek(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    // Adjust to Monday (tm_wday: 0=Sunday, 1=Monday, ..., 6=Saturday)
    int daysToSubtract = (tm.tm_wday == 0) ? 6 : (tm.tm_wday - 1);
    tm.tm_mday -= daysToSubtract;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return std::mktime(&tm);
}

int64_t TimeUtils::endOfWeek(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    // Adjust to Sunday (tm_wday: 0=Sunday, 1=Monday, ..., 6=Saturday)
    int daysToAdd = (tm.tm_wday == 0) ? 0 : (7 - tm.tm_wday);
    tm.tm_mday += daysToAdd;
    tm.tm_hour = 23;
    tm.tm_min = 59;
    tm.tm_sec = 59;
    return std::mktime(&tm);
}

int64_t TimeUtils::startOfMonth(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return std::mktime(&tm);
}

int64_t TimeUtils::endOfMonth(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    tm.tm_mon += 1;
    tm.tm_mday = 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return std::mktime(&tm) - 1;
}

std::tm TimeUtils::toLocalTime(int64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time);
#else
    localtime_r(&time, &tm);
#endif
    return tm;
}

int64_t TimeUtils::fromLocalTime(int year, int month, int day, int hour, int minute, int second) {
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;  // Let mktime determine DST
    return static_cast<int64_t>(std::mktime(&tm));
}

std::string TimeUtils::formatDuration(int64_t seconds) {
    auto& L = localization::L10n();

    if (seconds < 0) seconds = 0;

    int64_t hours = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;

    std::ostringstream oss;
    if (hours > 0) {
        oss << hours << L.get("h") << " " << minutes << L.get("min");
    } else {
        oss << minutes << L.get("min");
    }
    return oss.str();
}

std::string TimeUtils::formatDurationHMS(int64_t seconds) {
    if (seconds < 0) seconds = 0;

    int64_t hours = seconds / 3600;
    int64_t minutes = (seconds % 3600) / 60;
    int64_t secs = seconds % 60;

    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hours << ":"
        << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << secs;
    return oss.str();
}

std::string TimeUtils::formatDateTime(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M");
    return oss.str();
}

std::string TimeUtils::formatTime(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M");
    return oss.str();
}

std::string TimeUtils::formatDate(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d");
    return oss.str();
}

int64_t TimeUtils::parseDate(const std::string& dateStr) {
    std::tm tm{};
    std::istringstream iss(dateStr);
    iss >> std::get_time(&tm, "%Y-%m-%d");
    if (iss.fail()) {
        return 0;
    }
    tm.tm_isdst = -1;
    return static_cast<int64_t>(std::mktime(&tm));
}

int64_t TimeUtils::parseTime(const std::string& timeStr, int64_t dateTimestamp) {
    int hour = 0, minute = 0;
    if (sscanf(timeStr.c_str(), "%d:%d", &hour, &minute) != 2) {
        return dateTimestamp;
    }

    std::tm tm = toLocalTime(dateTimestamp);
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return static_cast<int64_t>(std::mktime(&tm));
}

int TimeUtils::getISOWeek(int64_t timestamp) {
    std::tm tm = toLocalTime(timestamp);
    char buf[8];
    std::strftime(buf, sizeof(buf), "%V", &tm);
    return std::stoi(buf);
}

int TimeUtils::getDaysInMonth(int year, int month) {
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month < 1 || month > 12) return 0;
    int days = daysInMonth[month - 1];
    if (month == 2 && isLeapYear(year)) {
        days = 29;
    }
    return days;
}

bool TimeUtils::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int TimeUtils::countWeekdays(int64_t startTime, int64_t endTime) {
    int count = 0;
    int64_t day = startOfDay(startTime);
    int64_t end = startOfDay(endTime);
    while (day <= end) {
        std::tm tm = toLocalTime(day);
        // tm_wday: 0=Sunday, 1=Monday, ..., 5=Friday, 6=Saturday
        if (tm.tm_wday >= 1 && tm.tm_wday <= 5) {
            count++;
        }
        tm.tm_mday += 1;
        tm.tm_isdst = -1;
        day = static_cast<int64_t>(std::mktime(&tm));
    }
    return count;
}

} // namespace timetracker::utils
