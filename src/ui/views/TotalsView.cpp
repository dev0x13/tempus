#include "TotalsView.hpp"
#include "utils/TimeUtils.hpp"
#include "utils/Platform.hpp"
#include "ui/widgets/DatePicker.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui {

TotalsView::TotalsView(
    std::shared_ptr<services::StatisticsService> statsService,
    std::shared_ptr<services::ExportService> exportService)
    : statsService_(std::move(statsService))
    , exportService_(std::move(exportService)) {
    // Default to this month
    int64_t now = utils::TimeUtils::now();
    displayStartTime_ = utils::TimeUtils::startOfMonth(now);
    displayEndTime_ = utils::TimeUtils::endOfDay(now);

    // Initialize date picker arrays
    widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
    widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);

    refreshStatistics();
}

void TotalsView::render() {
    ImGui::BeginChild("TotalsContent", ImVec2(0, 0), false);

    renderDateSelector();
    ImGui::Separator();
    renderTabs();

    ImGui::EndChild();
}

void TotalsView::renderDateSelector() {
    ImGui::Text("Date Range:");
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##totalsStartDate", displayStartDate_)) {
        displayStartTime_ = widgets::DatePicker::dateToTimestamp(displayStartDate_);
        refreshStatistics();
    }

    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##totalsEndDate", displayEndDate_)) {
        displayEndTime_ = utils::TimeUtils::endOfDay(widgets::DatePicker::dateToTimestamp(displayEndDate_));
        refreshStatistics();
    }

    // Quick date buttons
    ImGui::SameLine();
    if (ImGui::Button("This Week")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfWeek(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("This Month")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfMonth(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Last 30 Days")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfDay(now - 30 * 24 * 3600);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshStatistics();
    }

    renderExportButton();

    // Summary
    ImGui::Spacing();
    ImGui::Text("Total: %.1f hours (%d entries)", stats_.totalHours(), stats_.totalFacts);
}

void TotalsView::renderTabs() {
    if (ImGui::BeginTabBar("TotalsTabs")) {
        if (ImGui::BeginTabItem("By Activity")) {
            renderActivityTotals();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Daily")) {
            renderDailyTotals();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Weekly")) {
            renderWeeklyTotals();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Monthly")) {
            renderMonthlyTotals();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

void TotalsView::renderActivityTotals() {
    if (stats_.byActivity.empty()) {
        ImGui::TextDisabled("No data for this date range");
        return;
    }

    if (ImGui::BeginTable("ActivityTotals", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (const auto& total : stats_.byActivity) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", total.activityName.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", utils::TimeUtils::formatDuration(total.totalSeconds).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", total.count);
        }

        ImGui::EndTable();
    }
}

void TotalsView::renderDailyTotals() {
    if (stats_.daily.empty()) {
        ImGui::TextDisabled("No data for this date range");
        return;
    }

    if (ImGui::BeginTable("DailyTotals", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Date", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (const auto& total : stats_.daily) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", total.dateString().c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", utils::TimeUtils::formatDuration(total.totalSeconds).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", total.count);
        }

        ImGui::EndTable();
    }
}

void TotalsView::renderWeeklyTotals() {
    if (stats_.weekly.empty()) {
        ImGui::TextDisabled("No data for this date range");
        return;
    }

    if (ImGui::BeginTable("WeeklyTotals", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Week", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (const auto& total : stats_.weekly) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", total.weekString().c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", utils::TimeUtils::formatDuration(total.totalSeconds).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", total.count);
        }

        ImGui::EndTable();
    }
}

void TotalsView::renderMonthlyTotals() {
    if (stats_.monthly.empty()) {
        ImGui::TextDisabled("No data for this date range");
        return;
    }

    if (ImGui::BeginTable("MonthlyTotals", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Month", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Entries", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableHeadersRow();

        for (const auto& total : stats_.monthly) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", total.monthString().c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", utils::TimeUtils::formatDuration(total.totalSeconds).c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%d", total.count);
        }

        ImGui::EndTable();
    }
}

void TotalsView::renderExportButton() {
    ImGui::SameLine();
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - 100);
    if (ImGui::Button("Export CSV", ImVec2(100, 0))) {
        performExport();
    }
}

void TotalsView::performExport() {
    // Generate default filename with current date
    int64_t now = utils::TimeUtils::now();
    auto tm = utils::TimeUtils::toLocalTime(now);
    char defaultFilename[64];
    snprintf(defaultFilename, sizeof(defaultFilename), "time-tracker-export-%04d-%02d-%02d.csv",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    // Show native file save dialog
    auto selectedPath = utils::Platform::showSaveFileDialog(
        "Export to CSV",
        defaultFilename,
        {"CSV Files", "*.csv", "All Files", "*"}
    );

    // User cancelled the dialog
    if (!selectedPath.has_value()) {
        return;
    }

    // Export to selected path
    exportService_->exportToCsvFile(selectedPath->string(), displayStartTime_, displayEndTime_);
}

void TotalsView::refreshStatistics() {
    stats_ = statsService_->getStatistics(displayStartTime_, displayEndTime_);
    statsValid_ = true;
}

} // namespace timetracker::ui
