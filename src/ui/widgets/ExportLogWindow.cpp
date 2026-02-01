#include "ExportLogWindow.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <cstdio>

namespace timetracker::ui::widgets {

ExportLogWindow::ExportLogWindow(std::shared_ptr<repositories::IYouTrackExportLogRepository> repository)
    : repository_(std::move(repository)) {}

void ExportLogWindow::render() {
    if (!visible_) return;

    auto& L = localization::L10n();

    // Update pagination metadata on each render
    updatePaginationMetadata();

    // Center the window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_Appearing);

    if (ImGui::Begin(L.get("YouTrack Export Log"), &visible_, ImGuiWindowFlags_NoCollapse)) {
        if (totalEntries_ == 0) {
            // Empty state
            ImGui::TextWrapped("%s", L.get("No export log entries found. Export some activities to YouTrack to see them logged here."));
        } else {
            renderLogTable();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            renderPaginationControls();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        renderClearLogControls();

        ImGui::End();
    }
}

void ExportLogWindow::show() {
    visible_ = true;
    currentPage_ = 0;  // Reset to first page when opening
}

void ExportLogWindow::hide() {
    visible_ = false;
}

void ExportLogWindow::updatePaginationMetadata() {
    totalEntries_ = repository_->getTotalCount();
    totalPages_ = (totalEntries_ == 0) ? 0 : static_cast<int>(std::ceil(static_cast<double>(totalEntries_) / pageSize_));

    // Clamp current page to valid range
    if (currentPage_ >= totalPages_ && totalPages_ > 0) {
        currentPage_ = totalPages_ - 1;
    }
    if (currentPage_ < 0) {
        currentPage_ = 0;
    }
}

void ExportLogWindow::renderLogTable() {
    auto& L = localization::L10n();

    // Fetch current page entries
    int offset = currentPage_ * pageSize_;
    auto entries = repository_->getLogEntries(offset, pageSize_);

    // Table header
    if (ImGui::BeginTable("ExportLogTable", 5,
                         ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn(L.get("Export Time"), ImGuiTableColumnFlags_WidthFixed, 150.0f);
        ImGui::TableSetupColumn(L.get("Activity"), ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn(L.get("Issue ID"), ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn(L.get("Tracked Date"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn(L.get("Duration"), ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableHeadersRow();

        // Table rows
        for (const auto& entry : entries) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", formatTimestamp(entry.exportedAt).c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", entry.activityName.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entry.issueId.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", entry.trackedDate.c_str());

            ImGui::TableSetColumnIndex(4);
            int hours = entry.durationMinutes / 60;
            int minutes = entry.durationMinutes % 60;
            char durationStr[64];
            if (hours > 0) {
                snprintf(durationStr, sizeof(durationStr), "%d%s %d%s",
                    hours, L.get("h"), minutes, L.get("min"));
            } else {
                snprintf(durationStr, sizeof(durationStr), "%d%s",
                    minutes, L.get("min"));
            }
            ImGui::Text("%s", durationStr);
        }

        ImGui::EndTable();
    }
}

void ExportLogWindow::renderPaginationControls() {
    auto& L = localization::L10n();

    // Previous button
    if (currentPage_ == 0) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(L.get("< Previous"))) {
        if (currentPage_ > 0) {
            currentPage_--;
        }
    }
    if (currentPage_ == 0) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    // Page indicator
    char pageText[64];
    snprintf(pageText, sizeof(pageText), L.get("Page %d of %d"), currentPage_ + 1, totalPages_);
    ImGui::Text("%s", pageText);

    ImGui::SameLine();

    // Next button
    if (currentPage_ >= totalPages_ - 1) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(L.get("Next >"))) {
        if (currentPage_ < totalPages_ - 1) {
            currentPage_++;
        }
    }
    if (currentPage_ >= totalPages_ - 1) {
        ImGui::EndDisabled();
    }
}

void ExportLogWindow::renderClearLogControls() {
    auto& L = localization::L10n();

    if (ImGui::Button(L.get("Clear Log"))) {
        showClearConfirmation_ = true;
    }

    // Confirmation dialog
    if (showClearConfirmation_) {
        ImGui::OpenPopup(L.get("Clear Log Confirmation"));
    }

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(L.get("Clear Log Confirmation"), nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", L.get("Are you sure you want to clear all export log entries?"));
        ImGui::Text("%s", L.get("This action cannot be undone."));
        ImGui::Spacing();

        if (ImGui::Button(L.get("Yes, Clear Log"), ImVec2(150, 0))) {
            repository_->clearLog();
            showClearConfirmation_ = false;
            currentPage_ = 0;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button(L.get("Cancel"), ImVec2(150, 0))) {
            showClearConfirmation_ = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

std::string ExportLogWindow::formatTimestamp(int64_t timestamp) {
    std::time_t time = static_cast<std::time_t>(timestamp);
    std::tm* tm = std::localtime(&time);

    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace timetracker::ui::widgets
