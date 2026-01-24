#pragma once

#include <cstdint>

namespace timetracker::ui::widgets {

class TimeInput {
public:
    // Render a time input widget (HH:MM format)
    // Returns true if the time was changed
    // time: array of 2 ints [hour, minute]
    static bool render(const char* label, int* time);

    // Render a time input with seconds (HH:MM:SS format)
    static bool renderWithSeconds(const char* label, int* time); // [hour, minute, second]

    // Convert timestamp to time array [hour, minute]
    static void timestampToTime(int64_t timestamp, int* time);

    // Convert time array to seconds since midnight
    static int timeToSeconds(const int* time);

    // Validate and clamp time values
    static void validate(int* time);
};

} // namespace timetracker::ui::widgets
