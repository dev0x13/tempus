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
    // Set to date to today
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    toDate_[0] = tm->tm_year + 1900;
    toDate_[1] = tm->tm_mon + 1;
    toDate_[2] = tm->tm_mday;

    // Set from date to 30 days ago
    std::time_t thirtyDaysAgo = now - (30 * 24 * 60 * 60);
    tm = std::localtime(&thirtyDaysAgo);
    fromDate_[0] = tm->tm_year + 1900;
    fromDate_[1] = tm->tm_mon + 1;
    fromDate_[2] = tm->tm_mday;
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
        std::ostringstream oss;
        oss << L.KTalkImport.ImportSuccessMessage() << result.conferencesImported << " meeting(s)";
        statusMessage_ = oss.str();
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
    ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_Appearing);

    if (ImGui::Begin(L.KTalkImport.Title(), &visible_, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextWrapped("Import conference history from KTalk. Copy the fetch() request from your browser's DevTools Network tab.");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Date range selectors
        ImGui::Text("%s", L.KTalkImport.SelectDate());
        ImGui::Spacing();

        DatePicker::renderWithCalendar("From Date", fromDate_);
        ImGui::SameLine();
        ImGui::Text("  ");
        ImGui::SameLine();
        DatePicker::renderWithCalendar("To Date", toDate_);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Fetch payload text area
        ImGui::Text("Fetch Payload:");
        ImGui::TextWrapped("Paste the JavaScript fetch() call copied from browser DevTools:");
        ImGui::Spacing();

        ImGui::InputTextMultiline(
            "##fetchPayload",
            fetchPayload_,
            sizeof(fetchPayload_),
            ImVec2(-1, 200),
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
            ImGui::TextWrapped("Error: %s", statusMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // Buttons
        if (isImporting_) {
            ImGui::Text("Importing...");
        } else {
            if (ImGui::Button("Import", ImVec2(120, 0))) {
                handleImport();
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
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
