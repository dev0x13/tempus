#include "KTalkImportWindow.hpp"
#include "DatePicker.hpp"
#include "localization/LocalizationManager.hpp"
#include <imgui.h>
#include <ctime>
#include <sstream>
#include <iomanip>

namespace timetracker {
namespace ui {
namespace widgets {

KTalkImportWindow::KTalkImportWindow(std::shared_ptr<services::KTalkImportService> importService)
    : importService_(std::move(importService)) {
    fetchPayload_[0] = '\0';
    initializeDefaultDates();
}

void KTalkImportWindow::initializeDefaultDates() {
    // Set both dates to today
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    int year = tm->tm_year + 1900;
    int month = tm->tm_mon + 1;
    int day = tm->tm_mday;

    fromDate_[0] = year;
    fromDate_[1] = month;
    fromDate_[2] = day;

    toDate_[0] = year;
    toDate_[1] = month;
    toDate_[2] = day;
}

std::string KTalkImportWindow::dateToString(const int* date, bool isEndDate) {
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(4) << date[0] << "-"
        << std::setw(2) << date[1] << "-"
        << std::setw(2) << date[2];

    // Add time component: 00:00:00 for start date, 23:59:59 for end date
    if (isEndDate) {
        oss << " 23:59:59";
    } else {
        oss << " 00:00:00";
    }

    return oss.str();
}

void KTalkImportWindow::show() {
    visible_ = true;
    statusMessage_.clear();
    showSuccess_ = false;
    showError_ = false;
}

void KTalkImportWindow::hide() {
    visible_ = false;
}

void KTalkImportWindow::handleImport() {
    auto& L = localization::L10n();

    statusMessage_.clear();
    showSuccess_ = false;
    showError_ = false;
    isImporting_ = true;

    // Convert dates to strings with time components
    std::string fromDateStr = dateToString(fromDate_, false);  // 00:00:00
    std::string toDateStr = dateToString(toDate_, true);       // 23:59:59

    // Perform import
    services::ImportResult result = importService_->importConferences(
        std::string(fetchPayload_),
        fromDateStr,
        toDateStr
    );

    isImporting_ = false;

    if (result.success) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), L.get("Successfully imported %d conferences."), result.conferencesImported);
        statusMessage_ = buffer;
        showSuccess_ = true;

        // Clear form and close on success (after showing message)
        // User will see the success message for one frame before closing
    } else {
        statusMessage_ = result.errorMessage;
        showError_ = true;
    }
}

void KTalkImportWindow::render() {
    if (!visible_) {
        return;
    }

    auto& L = localization::L10n();

    // Center the window on first appearance
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::Begin(L.get("Import from KTalk"), &visible_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", L.get("Import conference history from KTalk. Copy the fetch() request from your browser's DevTools Network tab."));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Date range selectors
        ImGui::Text("%s", L.get("Select date to import:"));
        ImGui::Spacing();

        ImGui::Text("%s", L.get("From"));
        ImGui::SameLine();

        {
            int prevFrom[3] = {fromDate_[0], fromDate_[1], fromDate_[2]};
            if (DatePicker::renderWithCalendar(L.get("##displayFromDate"), fromDate_)) {
                // Shift toDate by the same delta as fromDate moved
                int64_t prevFromTs = DatePicker::dateToTimestamp(prevFrom);
                int64_t prevToTs   = DatePicker::dateToTimestamp(toDate_);
                int64_t delta      = prevToTs - prevFromTs;
                int64_t newFromTs  = DatePicker::dateToTimestamp(fromDate_);
                int64_t newToTs    = newFromTs + delta;
                DatePicker::timestampToDate(newToTs, toDate_);
            }
        }

        ImGui::SameLine();
        ImGui::Text("%s", L.get("to"));
        ImGui::SameLine();

        DatePicker::renderWithCalendar(L.get("##displayToDate"), toDate_);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Fetch payload text area
        ImGui::Text("%s", L.get("Fetch payload:"));
        ImGui::Spacing();

        ImGui::InputTextMultiline(
            "##fetchPayload",
            fetchPayload_,
            sizeof(fetchPayload_),
            ImVec2(300, 200),
            ImGuiInputTextFlags_None
        );

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Status messages
        if (showSuccess_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s", statusMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        } else if (showError_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
            ImGui::TextWrapped("%s: %s", L.get("Import failed"), statusMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // Buttons
        if (isImporting_) {
            ImGui::Text("%s", L.get("Importing..."));
        } else {
            if (ImGui::Button(L.get("Import"), ImVec2(120, 0))) {
                handleImport();
            }
            ImGui::SameLine();
            if (ImGui::Button(L.get("Cancel"), ImVec2(120, 0))) {
                hide();
            }

            // Auto-close after successful import
            if (showSuccess_) {
                // Close the window after showing success message
                hide();
            }
        }
    }
    ImGui::End();
}

}  // namespace widgets
}  // namespace ui
}  // namespace timetracker
