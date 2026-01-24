#include "TotalsView.hpp"
#include "utils/TimeUtils.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui {

TotalsView::TotalsView(
    std::shared_ptr<services::StatisticsService> statsService,
    std::shared_ptr<services::ExportService> exportService)
    : statsService_(std::move(statsService))
    , exportService_(std::move(exportService)) {
    memset(exportPath_, 0, sizeof(exportPath_));

    // Default to this month
    int64_t now = utils::TimeUtils::now();
    displayStartTime_ = utils::TimeUtils::startOfMonth(now);
    displayEndTime_ = utils::TimeUtils::endOfDay(now);

    // Set default export path
    auto tm = utils::TimeUtils::toLocalTime(now);
    snprintf(exportPath_, sizeof(exportPath_), "time-tracker-export-%04d%02d%02d.csv",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

    refreshStatistics();
}

void TotalsView::render() {
    ImGui::BeginChild("TotalsContent", ImVec2(0, 0), false);

    renderDateSelector();
    ImGui::Separator();
    renderTabs();

    if (showExportDialog_) {
        renderExportDialog();
    }

    ImGui::EndChild();
}

void TotalsView::renderDateSelector() {
    ImGui::Text("Date Range:");
    ImGui::SameLine();

    auto startTm = utils::TimeUtils::toLocalTime(displayStartTime_);
    auto endTm = utils::TimeUtils::toLocalTime(displayEndTime_);

    char startDateStr[32], endDateStr[32];
    snprintf(startDateStr, sizeof(startDateStr), "%04d-%02d-%02d",
             startTm.tm_year + 1900, startTm.tm_mon + 1, startTm.tm_mday);
    snprintf(endDateStr, sizeof(endDateStr), "%04d-%02d-%02d",
             endTm.tm_year + 1900, endTm.tm_mon + 1, endTm.tm_mday);

    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##startDate", startDateStr, sizeof(startDateStr), ImGuiInputTextFlags_EnterReturnsTrue)) {
        displayStartTime_ = utils::TimeUtils::parseDate(startDateStr);
        refreshStatistics();
    }

    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();

    ImGui::SetNextItemWidth(100);
    if (ImGui::InputText("##endDate", endDateStr, sizeof(endDateStr), ImGuiInputTextFlags_EnterReturnsTrue)) {
        displayEndTime_ = utils::TimeUtils::endOfDay(utils::TimeUtils::parseDate(endDateStr));
        refreshStatistics();
    }

    // Quick date buttons
    ImGui::SameLine();
    if (ImGui::Button("This Week")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfWeek(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        refreshStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("This Month")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfMonth(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        refreshStatistics();
    }
    ImGui::SameLine();
    if (ImGui::Button("Last 30 Days")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfDay(now - 30 * 24 * 3600);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
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
        showExportDialog_ = true;
    }
}

void TotalsView::renderExportDialog() {
    ImGui::OpenPopup("Export to CSV");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 150));

    if (ImGui::BeginPopupModal("Export to CSV", &showExportDialog_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Export Path:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##exportPath", exportPath_, sizeof(exportPath_));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Export", ImVec2(100, 0))) {
            if (exportService_->exportToCsvFile(exportPath_, displayStartTime_, displayEndTime_)) {
                showExportDialog_ = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            showExportDialog_ = false;
        }

        ImGui::EndPopup();
    }
}

void TotalsView::refreshStatistics() {
    stats_ = statsService_->getStatistics(displayStartTime_, displayEndTime_);
    statsValid_ = true;
}

} // namespace timetracker::ui
