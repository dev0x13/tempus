#include "TotalsView.hpp"
#include "utils/TimeUtils.hpp"
#include "utils/Platform.hpp"
#include "ui/widgets/DatePicker.hpp"
#include "imgui.h"
#include <cstring>
#include <thread>

namespace timetracker::ui {

TotalsView::TotalsView(
    std::shared_ptr<services::StatisticsService> statsService,
    std::shared_ptr<services::ExportService> exportService,
    std::shared_ptr<services::YouTrackExportService> youTrackExportService)
    : statsService_(std::move(statsService))
    , exportService_(std::move(exportService))
    , youTrackExportService_(std::move(youTrackExportService)) {
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

    // Render YouTrack export dialogs
    if (showExportConfirmation_) {
        renderExportConfirmationDialog();
    }
    if (showExportProgress_) {
        renderExportProgressDialog();
    }
    if (showExportSuccess_) {
        renderExportSuccessDialog();
    }
    if (showExportError_) {
        renderExportErrorDialog();
    }
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
    // Start export section on new line
    ImGui::Spacing();

    // YouTrack export button
    renderYouTrackExportButton();

    // CSV export button on same line
    ImGui::SameLine();
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

void TotalsView::renderYouTrackExportButton() {
    bool isConfigured = youTrackExportService_->isConfigured();

    if (!isConfigured) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Export to YouTrack", ImVec2(150, 0))) {
        performYouTrackExport();
    }

    if (!isConfigured) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", youTrackExportService_->getConfigurationError().c_str());
        }
    }
}

void TotalsView::performYouTrackExport() {
    // Prepare export data
    pendingWorkItems_ = youTrackExportService_->prepareExport(displayStartTime_, displayEndTime_);

    if (pendingWorkItems_.empty()) {
        exportErrorMessage_ = "No completed time entries found in selected date range.";
        showExportError_ = true;
        return;
    }

    // Show confirmation dialog
    showExportConfirmation_ = true;
}

void TotalsView::renderExportConfirmationDialog() {
    ImGui::OpenPopup("Confirm YouTrack Export");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Confirm YouTrack Export", &showExportConfirmation_)) {
        ImGui::Text("The following time entries will be exported to YouTrack:");
        ImGui::Separator();

        // Scrollable list of work items
        ImGui::BeginChild("WorkItemsList", ImVec2(0, 250), true);

        int totalMinutes = 0;
        for (const auto& item : pendingWorkItems_) {
            ImGui::Text("%s: %d min (%s)", item.issueId.c_str(), item.minutes, item.date.c_str());
            if (!item.activityName.empty() && item.activityName != item.issueId) {
                ImGui::SameLine();
                ImGui::TextDisabled("(activity: %s)", item.activityName.c_str());
            }
            totalMinutes += item.minutes;
        }

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::Text("Total: %.1f hours (%d minutes)", totalMinutes / 60.0f, totalMinutes);
        ImGui::Separator();

        // Action buttons
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            showExportConfirmation_ = false;
            showExportProgress_ = true;

            // Initialize progress tracking
            currentProgress_ = 0;
            totalProgress_ = static_cast<int>(pendingWorkItems_.size());
            cancelExport_.store(false);

            // Start async export in a separate thread
            std::thread([this]() {
                // Progress callback
                auto progressCallback = [this](int current, int total) {
                    currentProgress_ = current;
                    totalProgress_ = total;
                };

                // Perform export with progress callback and cancel flag
                auto result = youTrackExportService_->exportToYouTrack(
                    pendingWorkItems_,
                    progressCallback,
                    &cancelExport_
                );

                // Update UI state on main thread (will be picked up on next frame)
                showExportProgress_ = false;

                if (result.success) {
                    exportedMinutes_ = result.totalMinutes;
                    exportedItems_ = result.itemsExported;
                    showExportSuccess_ = true;
                    refreshStatistics();
                } else {
                    // Check if it was a cancellation
                    if (result.errorMessage == "Export cancelled by user") {
                        // Show cancellation message with partial export info
                        exportErrorMessage_ = "Export cancelled. " + std::to_string(result.itemsExported) +
                                             " of " + std::to_string(totalProgress_) + " activities exported.";
                        // Refresh to show the partial export
                        if (result.itemsExported > 0) {
                            refreshStatistics();
                        }
                    } else {
                        exportErrorMessage_ = result.errorMessage;
                    }
                    showExportError_ = true;
                }
            }).detach();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            showExportConfirmation_ = false;
        }

        ImGui::EndPopup();
    }
}

void TotalsView::renderExportProgressDialog() {
    ImGui::OpenPopup("Exporting to YouTrack");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal("Exporting to YouTrack", &open, ImGuiWindowFlags_NoResize)) {
        // Progress text
        ImGui::Text("Exporting activity %d of %d", currentProgress_, totalProgress_);
        ImGui::Spacing();

        // Progress bar
        float progress = totalProgress_ > 0 ? static_cast<float>(currentProgress_) / static_cast<float>(totalProgress_) : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        ImGui::Spacing();

        ImGui::Separator();

        // Cancel button
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            cancelExport_.store(true);
            showExportProgress_ = false;
        }

        ImGui::EndPopup();
    }

    // If user closed via X button
    if (!open) {
        cancelExport_.store(true);
        showExportProgress_ = false;
    }
}

void TotalsView::renderExportSuccessDialog() {
    ImGui::OpenPopup("Export Successful");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal("Export Successful", &showExportSuccess_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Successfully exported %.1f hours to YouTrack!", exportedMinutes_ / 60.0f);
        ImGui::Text("(%d work items)", exportedItems_);
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            showExportSuccess_ = false;
        }

        ImGui::EndPopup();
    }
}

void TotalsView::renderExportErrorDialog() {
    ImGui::OpenPopup("Export Failed");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 200), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Export Failed", &showExportError_)) {
        ImGui::TextWrapped("%s", exportErrorMessage_.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            showExportError_ = false;
        }

        ImGui::EndPopup();
    }
}

} // namespace timetracker::ui
