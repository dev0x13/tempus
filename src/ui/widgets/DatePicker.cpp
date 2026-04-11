#include "DatePicker.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include <string>

namespace timetracker::ui::widgets {

namespace {
    // Helper function to split comma-separated strings
    std::vector<std::string> splitByComma(const char* str) {
        std::vector<std::string> result;
        std::string current;
        for (const char* p = str; *p != '\0'; ++p) {
            if (*p == ',') {
                result.push_back(current);
                current.clear();
            } else {
                current += *p;
            }
        }
        if (!current.empty()) {
            result.push_back(current);
        }
        return result;
    }
}

bool DatePicker::render(const char* label, int* date) {
    bool changed = false;

    ImGui::PushID(label);

    // Year
    ImGui::SetNextItemWidth(60);
    if (ImGui::InputInt("##year", &date[0], 0)) {
        if (date[0] < 1970) date[0] = 1970;
        if (date[0] > 2100) date[0] = 2100;
        changed = true;
    }
    ImGui::SameLine();
    ImGui::Text("-");
    ImGui::SameLine();

    // Month
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##month", &date[1], 0)) {
        if (date[1] < 1) date[1] = 1;
        if (date[1] > 12) date[1] = 12;
        changed = true;
    }
    ImGui::SameLine();
    ImGui::Text("-");
    ImGui::SameLine();

    // Day
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##day", &date[2], 0)) {
        int maxDay = getDaysInMonth(date[0], date[1]);
        if (date[2] < 1) date[2] = 1;
        if (date[2] > maxDay) date[2] = maxDay;
        changed = true;
    }

    ImGui::SameLine();
    if (label[0] != '#') {
        ImGui::Text("%s", label);
    }

    ImGui::PopID();
    return changed;
}

bool DatePicker::renderWithCalendar(const char* label, int* date) {
    bool changed = false;

    ImGui::PushID(label);

    // Display current date as button
    char dateStr[32];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", date[0], date[1], date[2]);

    if (ImGui::Button(dateStr, ImVec2(100, 0))) {
        ImGui::OpenPopup("calendar_popup");
    }

    if (label[0] != '#') {
        ImGui::SameLine();
        ImGui::Text("%s", label);
    }

    renderCalendarPopup("calendar_popup", date, changed);

    ImGui::PopID();
    return changed;
}

void DatePicker::renderCalendarPopup(const char* popupId, int* date, bool& changed) {
    if (ImGui::BeginPopup(popupId)) {
        auto& L = localization::L10n();
        constexpr float kCalendarCellWidth = 32.0f;
        constexpr float kCalendarCellHeight = 28.0f;

        // Get localized month and weekday names
        auto months = splitByComma(L.get("January,February,March,April,May,June,July,August,September,October,November,December"));
        auto weekdays = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));
        int firstDayOfWeek = L.getInt("FirstDayOfWeek", 1);

        // Month/Year navigation
        if (ImGui::ArrowButton("##prev_month", ImGuiDir_Left)) {
            date[1]--;
            if (date[1] < 1) {
                date[1] = 12;
                date[0]--;
            }
            changed = true;
        }
        ImGui::SameLine();

        char monthYear[128];
        if (date[1] >= 1 && date[1] <= 12 && months.size() >= 12) {
            snprintf(monthYear, sizeof(monthYear), "%s %d", months[date[1] - 1].c_str(), date[0]);
        } else {
            snprintf(monthYear, sizeof(monthYear), "%d", date[0]);
        }
        ImGui::Text("%s", monthYear);

        ImGui::SameLine();
        if (ImGui::ArrowButton("##next_month", ImGuiDir_Right)) {
            date[1]++;
            if (date[1] > 12) {
                date[1] = 1;
                date[0]++;
            }
            changed = true;
        }

        date[2] = (std::min)(date[2], getDaysInMonth(date[0], date[1]));

        ImGui::Separator();

        if (ImGui::BeginTable("##calendar_grid", 7, ImGuiTableFlags_SizingFixedFit)) {
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetupColumn(nullptr, ImGuiTableColumnFlags_WidthFixed, kCalendarCellWidth);
            }
            ImGui::TableNextRow();
            for (int i = 0; i < 7; ++i) {
                ImGui::TableSetColumnIndex(i);
                if (weekdays.size() >= 7) {
                    int dayIndex = (firstDayOfWeek + i) % 7;
                    ImVec2 textSize = ImGui::CalcTextSize(weekdays[dayIndex].c_str());
                    float textOffset = (kCalendarCellWidth - textSize.x) * 0.5f;
                    if (textOffset > 0.0f) {
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textOffset);
                    }
                    ImGui::TextUnformatted(weekdays[dayIndex].c_str());
                } else {
                    ImGui::Dummy(ImVec2(kCalendarCellWidth, 0.0f));
                }
            }

            int daysInMonth = getDaysInMonth(date[0], date[1]);
            int firstDay = getFirstDayOfMonth(date[0], date[1]);
            int totalCells = firstDay + daysInMonth;
            int rowCount = (totalCells + 6) / 7;

            for (int row = 0; row < rowCount; ++row) {
                ImGui::TableNextRow();
                for (int col = 0; col < 7; ++col) {
                    ImGui::TableSetColumnIndex(col);
                    int cellIndex = row * 7 + col;
                    int day = cellIndex - firstDay + 1;

                    if (day < 1 || day > daysInMonth) {
                        ImGui::Dummy(ImVec2(kCalendarCellWidth, kCalendarCellHeight));
                        continue;
                    }

                    char dayStr[8];
                    snprintf(dayStr, sizeof(dayStr), "%d", day);

                    bool isSelected = (day == date[2]);
                    if (isSelected) {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.6f, 0.9f, 1.0f));
                    }

                    if (ImGui::Button(dayStr, ImVec2(kCalendarCellWidth, kCalendarCellHeight))) {
                        date[2] = day;
                        changed = true;
                        ImGui::CloseCurrentPopup();
                    }

                    if (isSelected) {
                        ImGui::PopStyleColor();
                    }
                }
            }

            ImGui::EndTable();
        }

        ImGui::EndPopup();
    }
}

void DatePicker::timestampToDate(int64_t timestamp, int* date) {
    auto tm = utils::TimeUtils::toLocalTime(timestamp);
    date[0] = tm.tm_year + 1900;
    date[1] = tm.tm_mon + 1;
    date[2] = tm.tm_mday;
}

int64_t DatePicker::dateToTimestamp(const int* date) {
    return utils::TimeUtils::fromLocalTime(date[0], date[1], date[2], 0, 0, 0);
}

int DatePicker::getDaysInMonth(int year, int month) {
    return utils::TimeUtils::getDaysInMonth(year, month);
}

int DatePicker::getFirstDayOfMonth(int year, int month) {
    auto& L = localization::L10n();
    int64_t timestamp = utils::TimeUtils::fromLocalTime(year, month, 1, 12, 0, 0);
    auto tm = utils::TimeUtils::toLocalTime(timestamp);

    // tm.tm_wday is 0=Sunday, 1=Monday, ..., 6=Saturday
    // Convert to calendar grid position based on locale's first day of week
    int firstDayOfWeek = L.getInt("FirstDayOfWeek", 1);
    return (tm.tm_wday - firstDayOfWeek + 7) % 7;
}

} // namespace timetracker::ui::widgets
