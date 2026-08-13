#define NOMINMAX
#include "TimeEntriesView.hpp"
#include "BlockScheduleRenderer.hpp"
#include "utils/TimeUtils.hpp"
#include "utils/Platform.hpp"
#include "ui/widgets/DatePicker.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <cstring>
#include <limits>
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
    std::shared_ptr<services::YouTrackExportService> youTrackExportService,
    std::shared_ptr<services::SettingsService> settingsService)
    : timeService_(std::move(timeService))
    , statsService_(std::move(statsService))
    , exportService_(std::move(exportService))
    , youTrackExportService_(std::move(youTrackExportService))
    , settingsService_(std::move(settingsService))
    , addAutocomplete_(timeService_) {

    scheduleRenderer_ = std::make_unique<BlockScheduleRenderer>(settingsService_, timeService_);

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
    if (timeService_->getDataRevision() != displayedDataRevision_) {
        refreshEntries();
    }

    visibleStats_ = statsService_->getStatistics(entries_);

    // Top button row
    renderTopButtons();
    ImGui::Separator();

    // Date selector
    renderDateSelector();
    ImGui::Separator();

    // Calculate footer height to fit its content exactly
    float footerHeight = 1.0f + ImGui::GetFrameHeightWithSpacing();  // separator + CollapsingHeader row
    if (showActivityBreakdown_) {
        footerHeight += visibleStats_.byActivity.size() * ImGui::GetTextLineHeightWithSpacing();
    }

    // Scrollable area for entries
    float availableHeight = ImGui::GetContentRegionAvail().y - footerHeight;
    ImGui::BeginChild("EntriesScrollArea", ImVec2(0, availableHeight), true);
    if (settingsService_->getBlockScheduleEnabled()) {
        renderBlockSchedule();
    } else {
        renderDateGroupedEntries();
    }
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

bool TimeEntriesView::requiresPeriodicRedraw() const {
    return showExportProgress_ || timeService_->isTracking();
}

double TimeEntriesView::periodicRedrawIntervalSeconds() const {
    if (showExportProgress_) {
        return 0.1;
    }
    if (timeService_->isTracking()) {
        return 1.0;
    }
    return 0.0;
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
    if (ImGui::Button(L.get("KTalk import"), ImVec2(130, 0))) {
        if (kTalkImportCallback_) {
            kTalkImportCallback_();
        }
    }

    // Auto-fill button
    ImGui::SameLine();
    if (ImGui::Button(L.get("Auto-fill"), ImVec2(110, 0))) {
        if (autoFillCallback_) {
            autoFillCallback_(displayStartTime_, displayEndTime_);
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

    int64_t rangeDuration = displayEndTime_ - displayStartTime_ + 1;

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
    if (ImGui::Button(L.get("This week"))) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfWeek(now);
        displayEndTime_ = utils::TimeUtils::endOfWeek(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button(L.get("Last 10 days"))) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfDay(now) - 9 * 86400;
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button("◀")) {
        displayStartTime_ -= rangeDuration;
        displayEndTime_ -= rangeDuration;
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button("▶")) {
        displayStartTime_ += rangeDuration;
        displayEndTime_ += rangeDuration;
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
        if (ImGui::BeginTable("EntriesTable", 7, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp)) {
            // Set up columns
            ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 50.0f);
            ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 40.0f);
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 120.0f);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn("YT", ImGuiTableColumnFlags_WidthFixed, 24.0f);
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 28.0f);

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
                ImGui::Text("%s", fact.activityName.c_str());

                // Description column
                ImGui::TableNextColumn();
                ImGui::Text("%s", fact.description.c_str());

                // Duration column (right-aligned)
                ImGui::TableNextColumn();
                float columnWidth = ImGui::GetContentRegionAvail().x;
                float textWidth = ImGui::CalcTextSize(duration.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columnWidth - textWidth);
                ImGui::Text("%s", duration.c_str());

                // YT export indicator column
                ImGui::TableNextColumn();
                if (fact.exportedToYoutrack) {
                    float fullLineHeight = ImGui::GetTextLineHeight();
                    ImGui::SetWindowFontScale(0.65f);
                    float ytColWidth = ImGui::GetContentRegionAvail().x;
                    float ytTextWidth = ImGui::CalcTextSize("YT").x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (ytColWidth - ytTextWidth) * 0.5f);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + (fullLineHeight - ImGui::GetTextLineHeight()) * 0.5f);
                    ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "YT");
                    ImGui::SetWindowFontScale(1.0f);
                }

                // Actions column
                ImGui::TableNextColumn();
                ImGui::SetWindowFontScale(0.6f);
                if (fact.endTime.has_value()) {
                    if (ImGui::Button("▶")) {
                        ImGui::SetWindowFontScale(1.0f);
                        timeService_->startTracking(fact.activityName);
                        refreshEntries();
                    }
                } else {
                    if (ImGui::Button("■")) {
                        ImGui::SetWindowFontScale(1.0f);
                        timeService_->stopTracking();
                        refreshEntries();
                    }
                }
                ImGui::SetWindowFontScale(1.0f);

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

void TimeEntriesView::renderBlockSchedule() {
    auto& L = localization::L10n();

    auto groupedEntries = groupEntriesByDate();

    if (groupedEntries.empty()) {
        // Still show empty schedule for the date range
        // Generate dates from displayStartTime_ to displayEndTime_
        int64_t cursor = displayStartTime_;
        while (cursor <= displayEndTime_) {
            auto tm = utils::TimeUtils::toLocalTime(cursor);

            // Build date header
            auto weekdayNames = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));
            auto monthNamesShort = splitByComma(L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec"));
            std::string weekday = (weekdayNames.size() > static_cast<size_t>(tm.tm_wday))
                ? weekdayNames[tm.tm_wday] : "";
            std::string month = (monthNamesShort.size() > static_cast<size_t>(tm.tm_mon))
                ? monthNamesShort[tm.tm_mon] : "";
            char dateStr[128];
            snprintf(dateStr, sizeof(dateStr), "%s %s.%d", weekday.c_str(), month.c_str(), tm.tm_mday);

            char isoKey[16];
            snprintf(isoKey, sizeof(isoKey), "%04d-%02d-%02d",
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

            int64_t dayStart = utils::TimeUtils::startOfDay(cursor);
            std::vector<models::Fact> emptyFacts;
            auto action = scheduleRenderer_->renderDay(dateStr, isoKey, emptyFacts, dayStart);

            if (action.type == ScheduleAction::EditFact && action.factToEdit) {
                startEdit(*action.factToEdit);
            } else if (action.type == ScheduleAction::CreateFact) {
                startAddWithTimes(action.slotStart, action.slotEnd);
            }

            cursor = dayStart + 24 * 3600;  // Next day
        }
        return;
    }

    // Build a map of ISO date -> (header, facts) for schedule rendering
    // We also need to generate empty days within the range
    std::map<std::string, std::pair<std::string, std::vector<models::Fact>>> dateMap;

    // First, populate from actual entries
    for (const auto& fact : entries_) {
        auto tm = utils::TimeUtils::toLocalTime(fact.startTime);
        char isoKey[16];
        snprintf(isoKey, sizeof(isoKey), "%04d-%02d-%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

        auto& entry = dateMap[isoKey];
        if (entry.first.empty()) {
            auto weekdayNames = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));
            auto monthNamesShort = splitByComma(L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec"));
            std::string weekday = (weekdayNames.size() > static_cast<size_t>(tm.tm_wday))
                ? weekdayNames[tm.tm_wday] : "";
            std::string month = (monthNamesShort.size() > static_cast<size_t>(tm.tm_mon))
                ? monthNamesShort[tm.tm_mon] : "";
            char dateStr[128];
            snprintf(dateStr, sizeof(dateStr), "%s %s.%d", weekday.c_str(), month.c_str(), tm.tm_mday);
            entry.first = dateStr;
        }
        entry.second.push_back(fact);
    }

    // Also generate entries for days without facts within the range
    int64_t cursor = displayStartTime_;
    while (cursor <= displayEndTime_) {
        auto tm = utils::TimeUtils::toLocalTime(cursor);
        char isoKey[16];
        snprintf(isoKey, sizeof(isoKey), "%04d-%02d-%02d",
                tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

        if (dateMap.find(isoKey) == dateMap.end()) {
            auto weekdayNames = splitByComma(L.get("Sun,Mon,Tue,Wed,Thu,Fri,Sat"));
            auto monthNamesShort = splitByComma(L.get("Jan,Feb,Mar,Apr,May,Jun,Jul,Aug,Sep,Oct,Nov,Dec"));
            std::string weekday = (weekdayNames.size() > static_cast<size_t>(tm.tm_wday))
                ? weekdayNames[tm.tm_wday] : "";
            std::string month = (monthNamesShort.size() > static_cast<size_t>(tm.tm_mon))
                ? monthNamesShort[tm.tm_mon] : "";
            char dateStr[128];
            snprintf(dateStr, sizeof(dateStr), "%s %s.%d", weekday.c_str(), month.c_str(), tm.tm_mday);
            dateMap[isoKey] = {dateStr, {}};
        }

        cursor = utils::TimeUtils::startOfDay(cursor) + 24 * 3600;
    }

    // Render in descending order (most recent first)
    for (auto it = dateMap.rbegin(); it != dateMap.rend(); ++it) {
        int64_t dayStart = utils::TimeUtils::parseDate(it->first);
        auto action = scheduleRenderer_->renderDay(
            it->second.first, it->first, it->second.second, dayStart);

        if (action.type == ScheduleAction::EditFact && action.factToEdit) {
            startEdit(*action.factToEdit);
        } else if (action.type == ScheduleAction::CreateFact) {
            startAddWithTimes(action.slotStart, action.slotEnd);
        }
    }
}

void TimeEntriesView::renderFixedFooter() {
    auto& L = localization::L10n();
    ImGui::Separator();

    // Calculate totals
    int64_t totalSeconds = 0;
    for (const auto& activity : visibleStats_.byActivity) {
        totalSeconds += activity.totalSeconds;
    }

    // Build footer label with optional overtime/undertime segment
    int64_t now = utils::TimeUtils::now();
    int64_t effectiveEndTime = std::min(displayEndTime_, utils::TimeUtils::endOfDay(now));
    int64_t expectedSeconds = statsService_->getExpectedSeconds(displayStartTime_, effectiveEndTime);
    std::string totalStr = std::string(L.get("Total")) + ": " + utils::TimeUtils::formatDuration(totalSeconds);
    if (expectedSeconds > 0) {
        int64_t balanceSeconds = totalSeconds - expectedSeconds;
        if (balanceSeconds > 0) {
            totalStr += std::string("  |  ") + L.get("Overtime") + ": " + utils::TimeUtils::formatDuration(balanceSeconds);
        } else if (balanceSeconds < 0) {
            totalStr += std::string("  |  ") + L.get("Undertime") + ": " + utils::TimeUtils::formatDuration(-balanceSeconds);
        }
    }
    showActivityBreakdown_ = ImGui::CollapsingHeader(totalStr.c_str());

    if (showActivityBreakdown_ && !visibleStats_.byActivity.empty()) {
        ImGui::Indent(20.0f);

        // Sort activities by total duration (descending)
        auto sortedActivities = visibleStats_.byActivity;
        std::sort(sortedActivities.begin(), sortedActivities.end(),
                  [](const models::ActivityTotal& a, const models::ActivityTotal& b) {
                      return a.totalSeconds > b.totalSeconds;
                  });

        for (const auto& activity : sortedActivities) {
            std::string durationStr = utils::TimeUtils::formatDuration(activity.totalSeconds);
            float durationWidth = ImGui::CalcTextSize(durationStr.c_str()).x;
            ImGui::Text("%s", activity.activityName.c_str());
            ImGui::SameLine(ImGui::GetWindowWidth() - durationWidth - 20.0f);
            ImGui::Text("%s", durationStr.c_str());
        }

        ImGui::Unindent(20.0f);
    }
}

void TimeEntriesView::renderEditForm() {
    auto& L = localization::L10n();

    ImGui::OpenPopup(L.get("Edit entry"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(L.get("Edit entry"), &showEditForm_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##editActivity", editActivityName_, sizeof(editActivityName_));
        // Fix for GNOME (#36)
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##editDescription", L.get("Optional description"), editActivityDescription_, sizeof(editActivityDescription_));
        // Fix for GNOME (#36)
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);

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
        if (editIsOngoing_) {
            ImGui::Checkbox(L.get("Ongoing"), &editIsOngoing_);
        }

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
            int64_t startTs = getTimestampFromDate(editStartDate_, editStartTime_);
            int64_t endTs = editIsOngoing_ ? std::numeric_limits<int64_t>::max() : getTimestampFromDate(editEndDate_, editEndTime_);
            int64_t now = utils::TimeUtils::now();
            if (startTs > now || startTs >= endTs) {
                editStartTimeError_ = true;
            } else {
                editStartTimeError_ = false;
                saveEdit();
                showEditForm_ = false;
            }
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

        if (editStartTimeError_) {
            ImGui::Spacing();
            int64_t startTs = getTimestampFromDate(editStartDate_, editStartTime_);
            int64_t now = utils::TimeUtils::now();
            if (startTs > now) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Start time cannot be in the future"));
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Start time must be before end time"));
            }
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderAddForm() {
    auto& L = localization::L10n();

    ImGui::OpenPopup(L.get("Add entry"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    //ImGui::SetNextWindowSize(ImVec2(400, 350));

    if (ImGui::BeginPopupModal(L.get("Add entry"), &showAddForm_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::SetNextItemWidth(-1);
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }
        addAutocomplete_.render("##addActivity", addActivityName_, sizeof(addActivityName_));
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);

        ImGui::SetNextItemWidth(-1);
        ImGui::InputTextWithHint("##addDescription",  L.get("Optional description"), addActivityDescription_, sizeof(addActivityDescription_));
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);

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
        if (!addFromBlock_) {
            ImGui::Checkbox(L.get("Ongoing"), &addIsOngoing_);
        }

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
            int64_t startTs = getTimestampFromDate(addStartDate_, addStartTime_);
            int64_t endTs = addIsOngoing_ ? std::numeric_limits<int64_t>::max() : getTimestampFromDate(addEndDate_, addEndTime_);
            int64_t now = utils::TimeUtils::now();
            if (strlen(addActivityName_) == 0) {
                addNameError_ = true;
                addStartTimeError_ = false;
            } else if (startTs > now || startTs >= endTs) {
                addNameError_ = false;
                addStartTimeError_ = true;
            } else {
                addNameError_ = false;
                addStartTimeError_ = false;
                saveAdd();
                showAddForm_ = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button(L.get("Cancel"), ImVec2(100, 0))) {
            showAddForm_ = false;
        }

        if (addNameError_) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Activity name is required"));
        }
        if (addStartTimeError_) {
            ImGui::Spacing();
            int64_t startTs = getTimestampFromDate(addStartDate_, addStartTime_);
            int64_t now = utils::TimeUtils::now();
            if (startTs > now) {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Start time cannot be in the future"));
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Start time must be before end time"));
            }
        }

        // Render the autocomplete dropdown. Inside a modal it appears as a top-level
        // window rendered after modal content, which keeps it on top of the modal.
        addAutocomplete_.renderDropdown();

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderExportConfirmationDialog() {
    auto& L = localization::L10n();
    ImGui::OpenPopup(L.get("Confirm YouTrack Export"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(500, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(L.get("Confirm YouTrack Export"), &showExportConfirmation_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", L.get("The following time entries will be exported to YouTrack:"));
        ImGui::Separator();

        // Scrollable list of work items
        ImGui::BeginChild("WorkItemsList", ImVec2(0, 250), true);

        int totalMinutes = 0;
        for (const auto& item : pendingWorkItems_) {
            ImGui::Text(L.get("%s: %d min (%s)"), item.issueId.c_str(), item.minutes, item.date.c_str());
            if (!item.activityName.empty() && item.activityName != item.issueId) {
                ImGui::SameLine();
                ImGui::TextDisabled(L.get("(activity: %s)"), item.activityName.c_str());
            }
            totalMinutes += item.minutes;
        }

        ImGui::EndChild();

        ImGui::Separator();
        std::string totalDuration = utils::TimeUtils::formatDuration(static_cast<int64_t>(totalMinutes) * 60);
        ImGui::Text(L.get("Total: %s"), totalDuration.c_str());

        if (skippedExportedCount_ > 0) {
            ImGui::TextDisabled(L.get("%d already exported entries were skipped."), skippedExportedCount_);
        }

        ImGui::Separator();

        // Action buttons
        if (ImGui::Button(L.get("Export"), ImVec2(120, 0))) {
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
                        auto& L2 = localization::L10n();
                        char cancelMsg[256];
                        snprintf(cancelMsg, sizeof(cancelMsg),
                            L2.get("Export cancelled. %d of %d activities exported."),
                            result.itemsExported, totalProgress_);
                        exportErrorMessage_ = std::string(cancelMsg);
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
        if (ImGui::Button(L.get("Cancel"), ImVec2(120, 0))) {
            showExportConfirmation_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderExportProgressDialog() {
    auto& L = localization::L10n();
    ImGui::OpenPopup(L.get("Exporting to YouTrack"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_Appearing);

    bool open = true;
    if (ImGui::BeginPopupModal(L.get("Exporting to YouTrack"), &open, ImGuiWindowFlags_NoResize)) {
        // Progress text
        ImGui::Text(L.get("Exporting activity %d of %d"), currentProgress_, totalProgress_);
        ImGui::Spacing();

        // Progress bar
        float progress = totalProgress_ > 0 ? static_cast<float>(currentProgress_) / static_cast<float>(totalProgress_) : 0.0f;
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));
        ImGui::Spacing();

        ImGui::Separator();

        // Cancel button
        if (ImGui::Button(L.get("Cancel"), ImVec2(120, 0))) {
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
    auto& L = localization::L10n();
    ImGui::OpenPopup(L.get("Export Successful"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(L.get("Export Successful"), &showExportSuccess_, ImGuiWindowFlags_AlwaysAutoResize)) {
        std::string exportedDuration = utils::TimeUtils::formatDuration(static_cast<int64_t>(exportedMinutes_) * 60);
        ImGui::Text(L.get("Successfully exported %s to YouTrack!"), exportedDuration.c_str());
        ImGui::Text(L.get("(%d tasks)"), exportedItems_);
        ImGui::Separator();

        if (ImGui::Button(L.get("OK"), ImVec2(120, 0))) {
            showExportSuccess_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderExportErrorDialog() {
    auto& L = localization::L10n();
    ImGui::OpenPopup(L.get("Export failed"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 0), ImGuiCond_Appearing);

    if (ImGui::BeginPopupModal(L.get("Export failed"), &showExportError_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", exportErrorMessage_.c_str());
        ImGui::Separator();

        if (ImGui::Button(L.get("OK"), ImVec2(120, 0))) {
            showExportError_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::renderOverlapErrorDialog() {
    auto& L = localization::L10n();
    ImGui::OpenPopup(L.get("Export failed"));

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    if (ImGui::BeginPopupModal(L.get("Export failed"), &showOverlapError_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextWrapped("%s", L.get("Overlapping activities detected."));
        ImGui::TextWrapped("%s", L.get("Please fix these entries manually before exporting:"));
        ImGui::Separator();

        // Scrollable list of overlapping facts
        ImGui::BeginChild("OverlappingFactsList", ImVec2(500, 250), true);

        for (const auto& fact : overlappingFacts_) {
            // Format: Activity Name: YYYY-MM-DD HH:MM - HH:MM
            std::string startTime = utils::TimeUtils::formatDateTime(fact.startTime);
            std::string endTime;
            if (fact.endTime.has_value()) {
                endTime = utils::TimeUtils::formatDateTime(*fact.endTime);
            } else {
                endTime = L.get("(ongoing)");
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

        if (ImGui::Button(L.get("OK"), ImVec2(120, 0))) {
            showOverlapError_ = false;
        }

        ImGui::EndPopup();
    }
}

void TimeEntriesView::refreshEntries() {
    entries_ = timeService_->getEntriesForRange(displayStartTime_, displayEndTime_);
    displayedDataRevision_ = timeService_->getDataRevision();
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

    editStartTimeError_ = false;
    showEditForm_ = true;
}

void TimeEntriesView::saveEdit() {
    if (!editingFact_.has_value()) return;

    models::Fact fact = *editingFact_;
    fact.startTime = getTimestampFromDate(editStartDate_, editStartTime_);
    // Editing an auto-filled block by hand makes it the user's — auto-fill must stop rebuilding it.
    fact.autoGenerated = false;

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
    addNameError_ = false;
    addStartTimeError_ = false;
    addAutocomplete_.clear();
    int64_t now = utils::TimeUtils::now();
    setDateFromTimestamp(now, addStartDate_, addStartTime_);  // Default to current time
    setDateFromTimestamp(now, addEndDate_, addEndTime_);  // Initialize end time (but ongoing by default)
    addIsOngoing_ = true;  // Default to ongoing
    addFromBlock_ = false;
    showAddForm_ = true;
}

void TimeEntriesView::startAddWithTimes(int64_t start, int64_t end) {
    memset(addActivityName_, 0, sizeof(addActivityName_));
    memset(addActivityDescription_, 0, sizeof(addActivityDescription_));
    addNameError_ = false;
    addStartTimeError_ = false;
    addAutocomplete_.clear();
    setDateFromTimestamp(start, addStartDate_, addStartTime_);
    setDateFromTimestamp(end, addEndDate_, addEndTime_);
    addIsOngoing_ = false;
    addFromBlock_ = true;
    showAddForm_ = true;
}

void TimeEntriesView::saveAdd() {
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
    auto& L = localization::L10n();

    // Check for overlapping facts first
    overlappingFacts_ = youTrackExportService_->checkForOverlaps(displayStartTime_, displayEndTime_);
    if (!overlappingFacts_.empty()) {
        showOverlapError_ = true;
        return;
    }

    // Prepare export data
    auto prepareResult = youTrackExportService_->prepareExport(displayStartTime_, displayEndTime_);

    if (!prepareResult.unresolvedActivities.empty()) {
        exportErrorMessage_ = std::string(L.get("The following activities have no YouTrack issue ID mapping:")) + "\n";
        for (const auto& name : prepareResult.unresolvedActivities) {
            exportErrorMessage_ += "  - " + name + "\n";
        }
        exportErrorMessage_ += L.get("Add aliases for these activities in Settings.");
        showExportError_ = true;
        return;
    }

    pendingWorkItems_ = std::move(prepareResult.workItems);
    skippedExportedCount_ = prepareResult.skippedExportedCount;

    if (pendingWorkItems_.empty()) {
        if (skippedExportedCount_ > 0) {
            exportErrorMessage_ = L.get("All entries in this range have already been exported to YouTrack.");
        } else {
            exportErrorMessage_ = L.get("No completed time entries found in selected date range.");
        }
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
