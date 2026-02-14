#include "TimeEntriesView.hpp"
#include "utils/TimeUtils.hpp"
#include "utils/Platform.hpp"
#include "ui/widgets/DatePicker.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <cstring>
#include <thread>
#include <algorithm>
#include <vector>
#include <string>

namespace timetracker::ui {

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

TimeEntriesView::TimeEntriesView(
    std::shared_ptr<services::TimeTrackingService> timeService,
    std::shared_ptr<services::StatisticsService> statsService,
    std::shared_ptr<services::ExportService> exportService,
    std::shared_ptr<services::YouTrackExportService> youTrackExportService)
    : timeService_(std::move(timeService))
    , statsService_(std::move(statsService))
    , exportService_(std::move(exportService))
    , youTrackExportService_(std::move(youTrackExportService)) {

    memset(editActivityName_, 0, sizeof(editActivityName_));
    memset(editActivityDescription_, 0, sizeof(editActivityDescription_));
    memset(addActivityName_, 0, sizeof(addActivityName_));
    memset(addActivityDescription_, 0, sizeof(addActivityDescription_));

    // Set default date range to today
    int64_t now = utils::TimeUtils::now();
    displayStartTime_ = utils::TimeUtils::startOfDay(now);
    displayEndTime_ = utils::TimeUtils::endOfDay(now);

    // Initialize date picker arrays
    widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
    widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);

    refreshEntries();
}

void TimeEntriesView::render() {
    // Refresh entries to pick up changes
    refreshEntries();

    // Top button row
    renderTopButtons();
    ImGui::Separator();

    // Date selector
    renderDateSelector();
    ImGui::Separator();

    // Calculate footer height (grand total + optional activity breakdown)
    float footerHeight = 40.0f;  // Space for grand total row
    if (showActivityBreakdown_) {
        auto stats = statsService_->getStatistics(displayStartTime_, displayEndTime_);
        footerHeight += stats.byActivity.size() * 20.0f + 20.0f;  // Estimate height
    }

    // Scrollable area for entries
    float availableHeight = ImGui::GetContentRegionAvail().y - footerHeight;
    ImGui::BeginChild("EntriesScrollArea", ImVec2(0, availableHeight), true);
    renderDateGroupedEntries();
    ImGui::EndChild();

    // Fixed footer with totals
    renderFixedFooter();

    // Modals
    if (showEditForm_) {
        renderEditForm();
    }

    if (showAddForm_) {
        renderAddForm();
    }

    // YouTrack export dialogs
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
    if (showOverlapError_) {
        renderOverlapErrorDialog();
    }
}

void TimeEntriesView::renderTopButtons() {
    auto& L = localization::L10n();

    // Start button
    if (ImGui::Button(L.get("Start"), ImVec2(80, 0))) {
        startAdd();
    }

    // Stop button (always visible)
    ImGui::SameLine();
    auto current = timeService_->getCurrentTracking();
    bool isTracking = current.has_value();
    if (!isTracking) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(L.get("Stop"), ImVec2(80, 0))) {
        timeService_->stopTracking();
        refreshEntries();
    }
    if (!isTracking) {
        ImGui::EndDisabled();
    }

    // Export CSV button
    ImGui::SameLine();
    if (ImGui::Button(L.get("Export CSV"), ImVec2(140, 0))) {
        performCsvExport();
    }

    // Export to YouTrack button
    ImGui::SameLine();
    bool isYouTrackConfigured = youTrackExportService_->isConfigured();
    if (!isYouTrackConfigured) {
        ImGui::BeginDisabled();
    }
    if (ImGui::Button(L.get("Export to YouTrack"), ImVec2(180, 0))) {
        performYouTrackExport();
    }
    if (!isYouTrackConfigured) {
        ImGui::EndDisabled();
        if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
            ImGui::SetTooltip("%s", youTrackExportService_->getConfigurationError().c_str());
        }
    }

    // KTalk Import button
    ImGui::SameLine();
    if (ImGui::Button(L.get("KTalk Import"), ImVec2(130, 0))) {
        if (kTalkImportCallback_) {
            kTalkImportCallback_();
        }
    }

    // Settings button
    ImGui::SameLine();
    if (ImGui::Button(L.get("Settings"), ImVec2(110, 0))) {
        if (settingsCallback_) {
            settingsCallback_();
        }
    }
}

void TimeEntriesView::renderDateSelector() {
    auto& L = localization::L10n();

    ImGui::Text("%s", L.get("From"));
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##displayStartDate", displayStartDate_)) {
        displayStartTime_ = widgets::DatePicker::dateToTimestamp(displayStartDate_);
        refreshEntries();
    }

    ImGui::SameLine();
    ImGui::Text("%s", L.get("to"));
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##displayEndDate", displayEndDate_)) {
        displayEndTime_ = utils::TimeUtils::endOfDay(widgets::DatePicker::dateToTimestamp(displayEndDate_));
        refreshEntries();
    }

    // Quick date buttons
    ImGui::SameLine();
    if (ImGui::Button(L.get("Today"))) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfDay(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button(L.get("This Week"))) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfWeek(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button(L.get("This Month"))) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfMonth(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
}

void TimeEntriesView::renderDateGroupedEntries() {
    auto& L = localization::L10n();

    if (entries_.empty()) {
        ImGui::TextDisabled("%s", L.get("No entries for selected date range"));
        return;
    }

    auto groupedEntries = groupEntriesByDate();
    int64_t now = utils::TimeUtils::now();

    for (const auto& [dateStr, dateEntries] : groupedEntries) {
        // Render date header
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", dateStr.c_str());
        ImGui::Spacing();

        // Render entries for this date using a table for proper alignment
        ImGui::Indent(20.0f);
        if (ImGui::BeginTable("EntriesTable", 4, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
            // Set up columns
            ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);

            for (const auto& fact : dateEntries) {
                ImGui::TableNextRow();

                // Format entry data
                std::string startTime = utils::TimeUtils::formatTime(fact.startTime);
                std::string endTime;
                if (fact.endTime.has_value()) {
                    endTime = utils::TimeUtils::formatTime(*fact.endTime);
                } else {
                    endTime = L.get("(ongoing)");
                }

                std::string duration = utils::TimeUtils::formatDuration(fact.getDuration(now));

                // Build activity display name with description
                std::string activityDisplay = fact.activityName;
                if (!fact.description.empty()) {
                    activityDisplay += " (" + fact.description + ")";
                }

                // Check if this fact overlaps with any other fact
                bool isOverlapping = isFactOverlapping(fact);

                // Set color for overlapping facts
                if (isOverlapping) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                }

                // Start time column
                ImGui::TableNextColumn();
                ImGui::PushID(fact.id);
                if (ImGui::Selectable(startTime.c_str(), false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                    startEdit(fact);
                }

                // End time column
                ImGui::TableNextColumn();
                ImGui::Text("%s", endTime.c_str());

                // Activity column
                ImGui::TableNextColumn();
                ImGui::Text("%s", activityDisplay.c_str());

                // Duration column (right-aligned)
                ImGui::TableNextColumn();
                float columnWidth = ImGui::GetContentRegionAvail().x;
                float textWidth = ImGui::CalcTextSize(duration.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columnWidth - textWidth);
                ImGui::Text("%s", duration.c_str());

                ImGui::PopID();

                if (isOverlapping) {
                    ImGui::PopStyleColor();
                }
            }

            ImGui::EndTable();
        }
        ImGui::Unindent(20.0f);

        ImGui::Spacing();
    }
}

void TimeEntriesView::renderFixedFooter() {
    auto& L = localization::L10n();
    ImGui::Separator();

    // Calculate totals
    auto stats = statsService_->getStatistics(displayStartTime_, displayEndTime_);
    int64_t totalSeconds = 0;
    for (const auto& activity : stats.byActivity) {
        totalSeconds += activity.totalSeconds;
    }

    // Grand total row (always visible, clickable)
    std::string totalStr = std::string(L.get("Total")) + ": " + utils::TimeUtils::formatDuration(totalSeconds);
    const char* arrow = showActivityBreakdown_ ? " ^" : " v";  // Simple ASCII arrows

    if (ImGui::Selectable((totalStr + arrow).c_str(), false, 0, ImVec2(0, 30))) {
        showActivityBreakdown_ = !showActivityBreakdown_;
    }

    // Activity breakdown (collapsible)
    if (showActivityBreakdown_ && !stats.byActivity.empty()) {
        ImGui::Separator();
        ImGui::Indent(20.0f);

        // Sort activities by total duration (descending)
        auto sortedActivities = stats.byActivity;
        std::sort(sortedActivities.begin(), sortedActivities.end(),
                  [](const models::ActivityTotal& a, const models::ActivityTotal& b) {
                      return a.totalSeconds > b.totalSeconds;
                  });

        for (const auto& activity : sortedActivities) {
            std::string durationStr = utils::TimeUtils::formatDuration(activity.totalSeconds);

            // Format: Activity Name                         Duration
            char activityLine[256];
            snprintf(activityLine, sizeof(activityLine), "%-50s  %s",
                     activity.activityName.c_str(), durationStr.c_str());

            ImGui::Text("%s", activityLine);
        }

        ImGui::Unindent(20.0f);
    }
}

void TimeEntriesView::renderEditForm() {
    auto& L = localization::L10n();

    ImGui::OpenPopup(L.get("Edit Entry"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));

    if (ImGui::BeginPopupModal(L.get("Edit Entry"), &showEditForm_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("%s", L.get("Activity:"));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##editActivity", editActivityName_, sizeof(editActivityName_));

        ImGui::Text("%s", L.get("Description:"));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##editDescription", editActivityDescription_, sizeof(editActivityDescription_));

        ImGui::Spacing();
        ImGui::Text("%s", L.get("Start:"));
        widgets::DatePicker::renderWithCalendar("##editStartDate", editStartDate_);
        ImGui::SameLine();

        // Format time with leading zeros
        char startHourBuf[8], startMinBuf[8];
        snprintf(startHourBuf, sizeof(startHourBuf), "%02d", editStartTime_[0]);
        snprintf(startMinBuf, sizeof(startMinBuf), "%02d", editStartTime_[1]);

        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##startHour", startHourBuf, sizeof(startHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(startHourBuf);
            editStartTime_[0] = (val < 0) ? 0 : (val > 23) ? 23 : val;
        }
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##startMin", startMinBuf, sizeof(startMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(startMinBuf);
            editStartTime_[1] = (val < 0) ? 0 : (val > 59) ? 59 : val;
        }

        ImGui::Spacing();
        ImGui::Checkbox(L.get("Ongoing"), &editIsOngoing_);

        if (!editIsOngoing_) {
            ImGui::Text("%s", L.get("End:"));
            widgets::DatePicker::renderWithCalendar("##editEndDate", editEndDate_);
            ImGui::SameLine();

            // Format time with leading zeros
            char endHourBuf[8], endMinBuf[8];
            snprintf(endHourBuf, sizeof(endHourBuf), "%02d", editEndTime_[0]);
            snprintf(endMinBuf, sizeof(endMinBuf), "%02d", editEndTime_[1]);

            ImGui::SetNextItemWidth(40);
            if (ImGui::InputText("##endHour", endHourBuf, sizeof(endHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
                int val = atoi(endHourBuf);
                editEndTime_[0] = (val < 0) ? 0 : (val > 23) ? 23 : val;
            }
            ImGui::SameLine();
            ImGui::Text(":");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40);
            if (ImGui::InputText("##endMin", endMinBuf, sizeof(endMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
                int val = atoi(endMinBuf);
                editEndTime_[1] = (val < 0) ? 0 : (val > 59) ? 59 : val;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(L.get("Save"), ImVec2(100, 0))) {
            saveEdit();
            showEditForm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(L.get("Delete"), ImVec2(100, 0))) {
            deleteEntry();
            showEditForm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(L.get("Cancel"), ImVec2(100, 0))) {
            showEditForm_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderAddForm() {
    auto& L = localization::L10n();

    ImGui::OpenPopup(L.get("Add Entry"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 350));

    if (ImGui::BeginPopupModal(L.get("Add Entry"), &showAddForm_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("%s", L.get("Activity:"));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##addActivity", addActivityName_, sizeof(addActivityName_));

        ImGui::Text("%s", L.get("Description:"));
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##addDescription", addActivityDescription_, sizeof(addActivityDescription_));

        ImGui::Spacing();
        ImGui::Text("%s", L.get("Start:"));
        widgets::DatePicker::renderWithCalendar("##addStartDate", addStartDate_);
        ImGui::SameLine();

        // Format time with leading zeros
        char addStartHourBuf[8], addStartMinBuf[8];
        snprintf(addStartHourBuf, sizeof(addStartHourBuf), "%02d", addStartTime_[0]);
        snprintf(addStartMinBuf, sizeof(addStartMinBuf), "%02d", addStartTime_[1]);

        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##addStartHour", addStartHourBuf, sizeof(addStartHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(addStartHourBuf);
            addStartTime_[0] = (val < 0) ? 0 : (val > 23) ? 23 : val;
        }
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##addStartMin", addStartMinBuf, sizeof(addStartMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(addStartMinBuf);
            addStartTime_[1] = (val < 0) ? 0 : (val > 59) ? 59 : val;
        }

        ImGui::Spacing();
        ImGui::Checkbox(L.get("Ongoing"), &addIsOngoing_);

        if (!addIsOngoing_) {
            ImGui::Text("%s", L.get("End:"));
            widgets::DatePicker::renderWithCalendar("##addEndDate", addEndDate_);
            ImGui::SameLine();

            // Format time with leading zeros
            char addEndHourBuf[8], addEndMinBuf[8];
            snprintf(addEndHourBuf, sizeof(addEndHourBuf), "%02d", addEndTime_[0]);
            snprintf(addEndMinBuf, sizeof(addEndMinBuf), "%02d", addEndTime_[1]);

            ImGui::SetNextItemWidth(40);
            if (ImGui::InputText("##addEndHour", addEndHourBuf, sizeof(addEndHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
                int val = atoi(addEndHourBuf);
                addEndTime_[0] = (val < 0) ? 0 : (val > 23) ? 23 : val;
            }
            ImGui::SameLine();
            ImGui::Text(":");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40);
            if (ImGui::InputText("##addEndMin", addEndMinBuf, sizeof(addEndMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
                int val = atoi(addEndMinBuf);
                addEndTime_[1] = (val < 0) ? 0 : (val > 59) ? 59 : val;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button(L.get("Add"), ImVec2(100, 0))) {
            saveAdd();
            showAddForm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button(L.get("Cancel"), ImVec2(100, 0))) {
            showAddForm_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderExportConfirmationDialog() {
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
                    refreshEntries();
                } else {
                    // Check if it was a cancellation
                    if (result.errorMessage == "Export cancelled by user") {
                        // Show cancellation message with partial export info
                        exportErrorMessage_ = "Export cancelled. " + std::to_string(result.itemsExported) +
                                             " of " + std::to_string(totalProgress_) + " activities exported.";
                        // Refresh to show the partial export
                        if (result.itemsExported > 0) {
                            refreshEntries();
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

void TimeEntriesView::renderExportProgressDialog() {
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

void TimeEntriesView::renderExportSuccessDialog() {
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

void TimeEntriesView::renderExportErrorDialog() {
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

void TimeEntriesView::renderOverlapErrorDialog() {
    ImGui::OpenPopup("Cannot Export: Overlapping Facts");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal("Cannot Export: Overlapping Facts", &showOverlapError_)) {
        ImGui::TextWrapped("Cannot export to YouTrack: overlapping facts detected.");
        ImGui::TextWrapped("Please fix these entries manually before exporting:");
        ImGui::Separator();

        // Scrollable list of overlapping facts
        ImGui::BeginChild("OverlappingFactsList", ImVec2(0, 250), true);

        for (const auto& fact : overlappingFacts_) {
            // Format: Activity Name: YYYY-MM-DD HH:MM - HH:MM
            std::string startTime = utils::TimeUtils::formatDateTime(fact.startTime);
            std::string endTime;
            if (fact.endTime.has_value()) {
                endTime = utils::TimeUtils::formatDateTime(*fact.endTime);
            } else {
                endTime = "(ongoing)";
            }

            // Build activity display name with description
            std::string activityDisplay = fact.activityName;
            if (!fact.description.empty()) {
                activityDisplay += " (" + fact.description + ")";
            }

            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", activityDisplay.c_str());
            ImGui::Text("  %s - %s", startTime.c_str(), endTime.c_str());
            ImGui::Spacing();
        }

        ImGui::EndChild();

        ImGui::Separator();
        ImGui::TextWrapped("Fix these entries by editing their start/end times so they no longer overlap.");

        if (ImGui::Button("OK", ImVec2(120, 0))) {
            showOverlapError_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::refreshEntries() {
    entries_ = timeService_->getEntriesForRange(displayStartTime_, displayEndTime_);
}

void TimeEntriesView::startEdit(const models::Fact& fact) {
    editingFact_ = fact;
    strncpy(editActivityName_, fact.activityName.c_str(), sizeof(editActivityName_) - 1);

    // Load fact description
    strncpy(editActivityDescription_, fact.description.c_str(), sizeof(editActivityDescription_) - 1);

    setDateFromTimestamp(fact.startTime, editStartDate_, editStartTime_);

    editIsOngoing_ = !fact.endTime.has_value();
    if (fact.endTime.has_value()) {
        setDateFromTimestamp(*fact.endTime, editEndDate_, editEndTime_);
    } else {
        setDateFromTimestamp(utils::TimeUtils::now(), editEndDate_, editEndTime_);
    }

    showEditForm_ = true;
}

void TimeEntriesView::saveEdit() {
    if (!editingFact_.has_value()) return;

    models::Fact fact = *editingFact_;
    fact.startTime = getTimestampFromDate(editStartDate_, editStartTime_);

    if (editIsOngoing_) {
        fact.endTime = std::nullopt;
    } else {
        fact.endTime = getTimestampFromDate(editEndDate_, editEndTime_);
    }

    timeService_->updateEntry(fact, editActivityName_, editActivityDescription_);
    refreshEntries();
}

void TimeEntriesView::deleteEntry() {
    if (editingFact_.has_value()) {
        timeService_->deleteEntry(editingFact_->id);
        refreshEntries();
    }
}

void TimeEntriesView::startAdd() {
    memset(addActivityName_, 0, sizeof(addActivityName_));
    memset(addActivityDescription_, 0, sizeof(addActivityDescription_));
    int64_t now = utils::TimeUtils::now();
    setDateFromTimestamp(now, addStartDate_, addStartTime_);  // Default to current time
    setDateFromTimestamp(now, addEndDate_, addEndTime_);  // Initialize end time (but ongoing by default)
    addIsOngoing_ = true;  // Default to ongoing
    showAddForm_ = true;
}

void TimeEntriesView::saveAdd() {
    if (strlen(addActivityName_) == 0) return;

    int64_t startTime = getTimestampFromDate(addStartDate_, addStartTime_);

    if (addIsOngoing_) {
        // Start tracking as ongoing entry
        timeService_->startTracking(addActivityName_, startTime, addActivityDescription_);
    } else {
        // Add completed entry with end time
        int64_t endTime = getTimestampFromDate(addEndDate_, addEndTime_);
        timeService_->addManualEntry(addActivityName_, startTime, endTime, addActivityDescription_);
    }
    refreshEntries();
}

void TimeEntriesView::performCsvExport() {
    // Generate default filename with current date
    int64_t now = utils::TimeUtils::now();
    auto tm = utils::TimeUtils::toLocalTime(now);
    char defaultFilename[64];
    snprintf(defaultFilename, sizeof(defaultFilename), "tempus-export-%04d-%02d-%02d.csv",
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

void TimeEntriesView::performYouTrackExport() {
    // Check for overlapping facts first
    overlappingFacts_ = youTrackExportService_->checkForOverlaps(displayStartTime_, displayEndTime_);
    if (!overlappingFacts_.empty()) {
        showOverlapError_ = true;
        return;
    }

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

void TimeEntriesView::setDateFromTimestamp(int64_t timestamp, int* date, int* time) {
    auto tm = utils::TimeUtils::toLocalTime(timestamp);
    date[0] = tm.tm_year + 1900;
    date[1] = tm.tm_mon + 1;
    date[2] = tm.tm_mday;
    time[0] = tm.tm_hour;
    time[1] = tm.tm_min;
}

int64_t TimeEntriesView::getTimestampFromDate(const int* date, const int* time) {
    return utils::TimeUtils::fromLocalTime(date[0], date[1], date[2], time[0], time[1], 0);
}

std::vector<std::pair<std::string, std::vector<models::Fact>>> TimeEntriesView::groupEntriesByDate() {
    auto& L = localization::L10n();

    // Parse localized weekday and month names
    auto weekdayNames = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));
    auto monthNamesShort = splitByComma(L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec"));

    // Group by ISO date key (YYYY-MM-DD) so the map sorts chronologically
    std::map<std::string, std::pair<std::string, std::vector<models::Fact>>> sortMap;

    for (const auto& fact : entries_) {
        auto tm = utils::TimeUtils::toLocalTime(fact.startTime);

        char isoKey[16];
        snprintf(isoKey, sizeof(isoKey), "%04d-%02d-%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

        auto& entry = sortMap[isoKey];
        if (entry.first.empty()) {
            std::string weekday = (weekdayNames.size() > static_cast<size_t>(tm.tm_wday))
                ? weekdayNames[tm.tm_wday] : "";
            std::string month = (monthNamesShort.size() > static_cast<size_t>(tm.tm_mon))
                ? monthNamesShort[tm.tm_mon] : "";
            char dateStr[128];
            snprintf(dateStr, sizeof(dateStr), "%s %s.%d",
                    weekday.c_str(), month.c_str(), tm.tm_mday);
            entry.first = dateStr;
        }
        entry.second.push_back(fact);
    }

    // Produce result in descending order (most recent first)
    std::vector<std::pair<std::string, std::vector<models::Fact>>> result;
    result.reserve(sortMap.size());
    for (auto it = sortMap.rbegin(); it != sortMap.rend(); ++it) {
        result.emplace_back(std::move(it->second.first), std::move(it->second.second));
    }
    return result;
}

bool TimeEntriesView::isFactOverlapping(const models::Fact& fact) const {
    // Skip ongoing facts
    if (!fact.endTime.has_value()) {
        return false;
    }

    // Check if this fact overlaps with any other fact in the entries list
    for (const auto& other : entries_) {
        // Skip comparing with itself
        if (fact.id == other.id) {
            continue;
        }

        // Check for overlap
        if (fact.overlapsWith(other)) {
            return true;
        }
    }

    return false;
}

} // namespace timetracker::ui
