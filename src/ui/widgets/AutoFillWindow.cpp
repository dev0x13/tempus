#include "AutoFillWindow.hpp"

#include "DatePicker.hpp"
#include "localization/LocalizationManager.hpp"
#include "utils/TimeUtils.hpp"

#include <imgui.h>

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace timetracker {
namespace ui {
namespace widgets {

namespace {

// Pinning the width matters: with AlwaysAutoResize alone, wrapped text and stretched table columns
// measure themselves against the window while the window measures itself against them, so the
// window creeps wider or narrower every frame.
constexpr float kWindowWidth = 540.0f;

// Same ceiling as AutoFillService::kMaxDaysPerRun.
constexpr int kMaxSelectableDays = 366;

// Beyond this the day grid scrolls instead of pushing the window off-screen.
constexpr int kMaxVisibleWeeks = 6;

std::vector<std::string> splitByComma(const char* text) {
    std::vector<std::string> parts;
    std::string current;
    for (const char* p = text; *p != '\0'; ++p) {
        if (*p == ',') {
            parts.push_back(current);
            current.clear();
        } else {
            current += *p;
        }
    }
    if (!current.empty()) {
        parts.push_back(current);
    }
    return parts;
}

int64_t nextLocalDay(int64_t dayStart) {
    std::tm tm = utils::TimeUtils::toLocalTime(dayStart);
    // mktime normalizes an out-of-range day, so this also steps over month and year ends.
    return utils::TimeUtils::fromLocalTime(tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday + 1);
}

bool isWeekend(int weekday) {
    return weekday == 0 || weekday == 6;
}

}  // namespace

AutoFillWindow::AutoFillWindow(std::shared_ptr<services::AutoFillService> autoFillService,
                               std::shared_ptr<services::SettingsService> settingsService)
    : autoFillService_(std::move(autoFillService)), settingsService_(std::move(settingsService)) {
    DatePicker::timestampToDate(utils::TimeUtils::now(), fromDate_);
    DatePicker::timestampToDate(utils::TimeUtils::now(), toDate_);
}

void AutoFillWindow::loadSettings() {
    std::string dayStart = settingsService_->getAutoFillDayStart();
    int hour = 9;
    int minute = 0;
    if (sscanf(dayStart.c_str(), "%d:%d", &hour, &minute) == 2) {
        dayStartHour_ = std::clamp(hour, 0, 23);
        dayStartMin_ = std::clamp(minute, 0, 59);
    }

    availableHours_ = static_cast<float>(settingsService_->getAutoFillAvailableMinutes()) / 60.0f;
    gridMinutes_ = settingsService_->getAutoFillGridMinutes();
    minBlockMinutes_ = settingsService_->getAutoFillMinBlockMinutes();

    rows_.clear();
    for (const auto& allocation : settingsService_->getAutoFillProfile()) {
        AutoFillRow row{};
        strncpy(row.activityName, allocation.activityName.c_str(), sizeof(row.activityName) - 1);
        row.percent = allocation.percent;
        rows_.push_back(row);
    }
    if (rows_.empty()) {
        rows_.push_back(AutoFillRow{});
    }
}

void AutoFillWindow::saveSettings() {
    char dayStart[8];
    snprintf(dayStart, sizeof(dayStart), "%02d:%02d", dayStartHour_, dayStartMin_);
    settingsService_->setAutoFillDayStart(dayStart);
    settingsService_->setAutoFillAvailableMinutes(static_cast<int>(std::lround(availableHours_ * 60.0f)));
    settingsService_->setAutoFillGridMinutes(gridMinutes_);
    settingsService_->setAutoFillMinBlockMinutes(minBlockMinutes_);

    models::AutoFillProfile profile;
    for (const auto& row : rows_) {
        if (row.activityName[0] == '\0') {
            continue;
        }
        profile.push_back({std::string(row.activityName), row.percent});
    }
    settingsService_->setAutoFillProfile(profile);
}

void AutoFillWindow::show(int64_t rangeStart, int64_t rangeEnd) {
    visible_ = true;
    statusMessage_.clear();
    showSuccess_ = false;
    showError_ = false;

    DatePicker::timestampToDate(rangeStart, fromDate_);
    DatePicker::timestampToDate(rangeEnd, toDate_);

    days_.clear();  // a fresh range starts from the default ticks, not the previous session's
    rebuildDayList();

    loadSettings();
}

void AutoFillWindow::rebuildDayList() {
    // Ticks the user already set survive a change of the range.
    const std::vector<AutoFillDay> previous = days_;
    days_.clear();

    int64_t cursor = DatePicker::dateToTimestamp(fromDate_);
    const int64_t lastDay = DatePicker::dateToTimestamp(toDate_);

    int guard = 0;
    while (cursor <= lastDay && guard < kMaxSelectableDays) {
        const std::tm tm = utils::TimeUtils::toLocalTime(cursor);

        AutoFillDay day;
        day.dayStart = cursor;
        day.weekday = tm.tm_wday;
        day.dayOfMonth = tm.tm_mday;
        day.month = tm.tm_mon + 1;
        // Weekends start off — a month-long range would otherwise need eight unticks to be usable.
        day.enabled = !isWeekend(day.weekday);

        for (const auto& old : previous) {
            if (old.dayStart == day.dayStart) {
                day.enabled = old.enabled;
                break;
            }
        }

        days_.push_back(day);
        cursor = nextLocalDay(cursor);
        ++guard;
    }
}

int AutoFillWindow::enabledDayCount() const {
    int count = 0;
    for (const auto& day : days_) {
        if (day.enabled) {
            ++count;
        }
    }
    return count;
}

void AutoFillWindow::hide() {
    visible_ = false;
}

int AutoFillWindow::totalPercent() const {
    int total = 0;
    for (const auto& row : rows_) {
        if (row.activityName[0] != '\0') {
            total += row.percent;
        }
    }
    return total;
}

void AutoFillWindow::handleApply() {
    auto& L = localization::L10n();

    statusMessage_.clear();
    showSuccess_ = false;
    showError_ = false;

    saveSettings();

    services::AutoFillOptions options;
    options.rangeStart = DatePicker::dateToTimestamp(fromDate_);
    options.rangeEnd = DatePicker::dateToTimestamp(toDate_);
    options.dayStartMinutes = dayStartHour_ * 60 + dayStartMin_;
    options.availableMinutes = static_cast<int>(std::lround(availableHours_ * 60.0f));
    options.gridMinutes = gridMinutes_;
    options.minBlockMinutes = minBlockMinutes_;
    options.allocations = settingsService_->getAutoFillProfile();

    for (const auto& day : days_) {
        if (day.enabled) {
            options.enabledDays.push_back(day.dayStart);
        }
    }
    // An empty list means "every day" to the service, so guard it here instead.
    if (options.enabledDays.empty()) {
        statusMessage_ = L.get("Tick at least one day to fill");
        showError_ = true;
        return;
    }

    services::AutoFillResult result = autoFillService_->run(options);

    if (!result.success) {
        statusMessage_ = result.errorMessage;
        showError_ = true;
        return;
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), L.get("Filled %d days, created %d entries"),
             result.daysProcessed, result.blocksCreated);
    statusMessage_ = buffer;

    if (result.factsClipped > 0) {
        snprintf(buffer, sizeof(buffer), L.get("Trimmed %d overlapping entries"), result.factsClipped);
        statusMessage_ += "\n";
        statusMessage_ += buffer;
    }
    if (result.daysSkippedOngoing > 0) {
        snprintf(buffer, sizeof(buffer), L.get("Skipped %d days with a running entry"), result.daysSkippedOngoing);
        statusMessage_ += "\n";
        statusMessage_ += buffer;
    }
    if (result.daysAlreadyFull > 0) {
        snprintf(buffer, sizeof(buffer), L.get("%d days already met the daily total, nothing added"), result.daysAlreadyFull);
        statusMessage_ += "\n";
        statusMessage_ += buffer;
    }
    if (result.daysShortOfBudget > 0) {
        snprintf(buffer, sizeof(buffer), L.get("%d days ran out of free time before the daily total was reached"), result.daysShortOfBudget);
        statusMessage_ += "\n";
        statusMessage_ += buffer;
    }
    showSuccess_ = true;
}

void AutoFillWindow::renderWindowSettings() {
    auto& L = localization::L10n();

    ImGui::Text("%s", L.get("Do not start before"));
    ImGui::SameLine();

    char hourBuf[8];
    char minBuf[8];
    snprintf(hourBuf, sizeof(hourBuf), "%02d", dayStartHour_);
    snprintf(minBuf, sizeof(minBuf), "%02d", dayStartMin_);

    ImGui::SetNextItemWidth(40);
    if (ImGui::InputText("##autofillStartHour", hourBuf, sizeof(hourBuf), ImGuiInputTextFlags_CharsDecimal)) {
        dayStartHour_ = std::clamp(atoi(hourBuf), 0, 23);
    }
    ImGui::SameLine();
    ImGui::Text(":");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(40);
    if (ImGui::InputText("##autofillStartMin", minBuf, sizeof(minBuf), ImGuiInputTextFlags_CharsDecimal)) {
        dayStartMin_ = std::clamp(atoi(minBuf), 0, 59);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("New entries are never placed before this time. Entries that already exist earlier still count toward the daily total."));
    }

    ImGui::Spacing();
    ImGui::Text("%s", L.get("Hours to log per day"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(120);
    if (ImGui::InputFloat("##autofillHours", &availableHours_, 0.5f, 1.0f, "%.1f")) {
        availableHours_ = std::clamp(availableHours_, 0.25f, 24.0f);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("The total a day should add up to, overtime included. Existing entries are subtracted from it, and only the difference is added."));
    }
}

void AutoFillWindow::renderDaySelection() {
    auto& L = localization::L10n();

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("Days to fill"));
    ImGui::Spacing();

    if (days_.empty()) {
        ImGui::TextDisabled("%s", L.get("Start date must not be after end date"));
        return;
    }

    if (ImGui::SmallButton(L.get("All"))) {
        for (auto& day : days_) {
            day.enabled = true;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(L.get("None"))) {
        for (auto& day : days_) {
            day.enabled = false;
        }
    }
    ImGui::SameLine();
    if (ImGui::SmallButton(L.get("Weekdays"))) {
        for (auto& day : days_) {
            day.enabled = !isWeekend(day.weekday);
        }
    }
    ImGui::SameLine();
    ImGui::Text(L.get("(%d of %d selected)"), enabledDayCount(), static_cast<int>(days_.size()));

    ImGui::Spacing();

    const int firstDayOfWeek = L.getInt("FirstDayOfWeek", 1);
    const auto weekdayNames = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));

    // Blank cells so every column is one weekday, the way a calendar reads.
    const int leadingCells = (days_.front().weekday - firstDayOfWeek + 7) % 7;
    const int weeks = static_cast<int>((leadingCells + days_.size() + 6) / 7);
    const int visibleWeeks = std::min(weeks, kMaxVisibleWeeks);
    const float gridHeight = ImGui::GetFrameHeightWithSpacing() * static_cast<float>(visibleWeeks + 1);

    if (ImGui::BeginChild("##autofillDayGrid", ImVec2(0, gridHeight), true)) {
        if (ImGui::BeginTable("AutoFillDays", 7, ImGuiTableFlags_SizingStretchSame)) {
            for (int i = 0; i < 7; ++i) {
                const int index = (firstDayOfWeek + i) % 7;
                ImGui::TableSetupColumn(weekdayNames.size() >= 7 ? weekdayNames[index].c_str() : "");
            }
            ImGui::TableHeadersRow();

            ImGui::TableNextRow();
            for (int i = 0; i < leadingCells; ++i) {
                ImGui::TableNextColumn();
            }

            for (std::size_t i = 0; i < days_.size(); ++i) {
                ImGui::TableNextColumn();  // wraps to the next row on its own past the last column

                ImGui::PushID(static_cast<int>(i));
                char label[16];
                snprintf(label, sizeof(label), "%d.%02d", days_[i].dayOfMonth, days_[i].month);

                const bool weekend = isWeekend(days_[i].weekday);
                if (weekend) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.6f, 0.6f, 1.0f));
                }
                ImGui::Checkbox(label, &days_[i].enabled);
                if (weekend) {
                    ImGui::PopStyleColor();
                }
                ImGui::PopID();
            }

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();
}

void AutoFillWindow::renderAllocations() {
    auto& L = localization::L10n();

    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("Share of free time"));
    ImGui::Spacing();
    ImGui::TextWrapped("%s", L.get("Percentages split only the time being added, that is the daily total minus what is already logged."));
    ImGui::Spacing();

    if (ImGui::BeginTable("AutoFillAllocations", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn(L.get("Activity name"), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(L.get("Percent"), ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < rows_.size(); ++i) {
            if (rows_[i].markedForDeletion) {
                continue;
            }

            ImGui::PushID(static_cast<int>(i));
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##activity", rows_[i].activityName, sizeof(rows_[i].activityName));

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-1);
            // Steps of 0 hide the -/+ buttons; the field is typed into, not nudged.
            if (ImGui::InputInt("##percent", &rows_[i].percent, 0, 0)) {
                rows_[i].percent = std::clamp(rows_[i].percent, 0, 100);
            }

            ImGui::TableNextColumn();
            if (ImGui::Button("X", ImVec2(30, 0))) {
                rows_[i].markedForDeletion = true;
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();
    if (ImGui::Button(L.get("+ Add activity"), ImVec2(180, 0))) {
        rows_.push_back(AutoFillRow{});
    }

    rows_.erase(std::remove_if(rows_.begin(), rows_.end(),
                               [](const AutoFillRow& row) { return row.markedForDeletion; }),
                rows_.end());

    ImGui::Spacing();
    const int total = totalPercent();
    if (total == 100) {
        ImGui::Text(L.get("Total: %d%%"), total);
    } else {
        // Not an error: the shares are normalized against their own sum.
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.3f, 1.0f));
        ImGui::Text(L.get("Total: %d%% (will be scaled to 100%%)"), total);
        ImGui::PopStyleColor();
    }
}

void AutoFillWindow::renderAdvancedSettings() {
    auto& L = localization::L10n();

    if (!ImGui::CollapsingHeader(L.get("Advanced"))) {
        return;
    }

    ImGui::Text("%s", L.get("Grid step (minutes)"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##autofillGrid", &gridMinutes_, 5, 15)) {
        gridMinutes_ = std::clamp(gridMinutes_, 1, 60);
    }

    ImGui::Text("%s", L.get("Minimum block (minutes)"));
    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::InputInt("##autofillMinBlock", &minBlockMinutes_, 5, 15)) {
        minBlockMinutes_ = std::clamp(minBlockMinutes_, 1, 480);
    }
}

void AutoFillWindow::render() {
    if (!visible_) {
        return;
    }

    auto& L = localization::L10n();

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    // Height still auto-fits; only the width is nailed down.
    ImGui::SetNextWindowSizeConstraints(ImVec2(kWindowWidth, 0.0f), ImVec2(kWindowWidth, FLT_MAX));

    if (ImGui::Begin(L.get("Auto-fill"), &visible_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        bool rangeChanged = false;

        ImGui::Text("%s", L.get("From"));
        ImGui::SameLine();
        {
            int prevFrom[3] = {fromDate_[0], fromDate_[1], fromDate_[2]};
            if (DatePicker::renderWithCalendar("##autofillFromDate", fromDate_)) {
                // Keep the range length stable when the start moves.
                int64_t delta = DatePicker::dateToTimestamp(toDate_) - DatePicker::dateToTimestamp(prevFrom);
                DatePicker::timestampToDate(DatePicker::dateToTimestamp(fromDate_) + delta, toDate_);
                rangeChanged = true;
            }
        }
        ImGui::SameLine();
        ImGui::Text("%s", L.get("to"));
        ImGui::SameLine();
        if (DatePicker::renderWithCalendar("##autofillToDate", toDate_)) {
            rangeChanged = true;
        }

        if (rangeChanged) {
            rebuildDayList();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderDaySelection();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderWindowSettings();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderAllocations();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderAdvancedSettings();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (showSuccess_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s", statusMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        } else if (showError_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s: %s", L.get("Auto-fill failed"), statusMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        if (ImGui::Button(L.get("Apply"), ImVec2(120, 0))) {
            handleApply();
        }
        ImGui::SameLine();
        if (ImGui::Button(L.get(showSuccess_ ? "Close" : "Cancel"), ImVec2(120, 0))) {
            hide();
        }
    }
    ImGui::End();

    if (visible_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        hide();
    }
}

}  // namespace widgets
}  // namespace ui
}  // namespace timetracker
