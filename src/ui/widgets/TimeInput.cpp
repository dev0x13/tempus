#include "TimeInput.hpp"
#include "utils/TimeUtils.hpp"
#include "imgui.h"

namespace timetracker::ui::widgets {

bool TimeInput::render(const char* label, int* time) {
    bool changed = false;

    ImGui::PushID(label);

    // Hour
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##hour", &time[0], 0)) {
        validate(time);
        changed = true;
    }

    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();

    // Minute
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##minute", &time[1], 0)) {
        validate(time);
        changed = true;
    }

    if (label[0] != '#') {
        ImGui::SameLine();
        ImGui::Text("%s", label);
    }

    ImGui::PopID();
    return changed;
}

bool TimeInput::renderWithSeconds(const char* label, int* time) {
    bool changed = false;

    ImGui::PushID(label);

    // Hour
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##hour", &time[0], 0)) {
        if (time[0] < 0) time[0] = 0;
        if (time[0] > 23) time[0] = 23;
        changed = true;
    }

    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();

    // Minute
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##minute", &time[1], 0)) {
        if (time[1] < 0) time[1] = 0;
        if (time[1] > 59) time[1] = 59;
        changed = true;
    }

    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();

    // Second
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputInt("##second", &time[2], 0)) {
        if (time[2] < 0) time[2] = 0;
        if (time[2] > 59) time[2] = 59;
        changed = true;
    }

    if (label[0] != '#') {
        ImGui::SameLine();
        ImGui::Text("%s", label);
    }

    ImGui::PopID();
    return changed;
}

void TimeInput::timestampToTime(int64_t timestamp, int* time) {
    auto tm = utils::TimeUtils::toLocalTime(timestamp);
    time[0] = tm.tm_hour;
    time[1] = tm.tm_min;
}

int TimeInput::timeToSeconds(const int* time) {
    return time[0] * 3600 + time[1] * 60;
}

void TimeInput::validate(int* time) {
    if (time[0] < 0) time[0] = 0;
    if (time[0] > 23) time[0] = 23;
    if (time[1] < 0) time[1] = 0;
    if (time[1] > 59) time[1] = 59;
}

} // namespace timetracker::ui::widgets
