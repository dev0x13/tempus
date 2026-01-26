#include "EditorView.hpp"
#include "utils/TimeUtils.hpp"
#include "ui/widgets/DatePicker.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui {

EditorView::EditorView(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {
    memset(editActivityName_, 0, sizeof(editActivityName_));
    memset(addActivityName_, 0, sizeof(addActivityName_));

    // Set default date range to today
    int64_t now = utils::TimeUtils::now();
    displayStartTime_ = utils::TimeUtils::startOfDay(now);
    displayEndTime_ = utils::TimeUtils::endOfDay(now);

    // Initialize date picker arrays
    widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
    widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);

    refreshEntries();
}

void EditorView::render() {
    ImGui::BeginChild("EditorContent", ImVec2(0, 0), false);

    // Date range selector
    ImGui::Text("Date Range:");
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##displayStartDate", displayStartDate_)) {
        displayStartTime_ = widgets::DatePicker::dateToTimestamp(displayStartDate_);
        refreshEntries();
    }

    ImGui::SameLine();
    ImGui::Text("to");
    ImGui::SameLine();

    if (widgets::DatePicker::renderWithCalendar("##displayEndDate", displayEndDate_)) {
        displayEndTime_ = utils::TimeUtils::endOfDay(widgets::DatePicker::dateToTimestamp(displayEndDate_));
        refreshEntries();
    }

    // Quick date buttons
    ImGui::SameLine();
    if (ImGui::Button("Today")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfDay(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button("This Week")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfWeek(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }
    ImGui::SameLine();
    if (ImGui::Button("This Month")) {
        int64_t now = utils::TimeUtils::now();
        displayStartTime_ = utils::TimeUtils::startOfMonth(now);
        displayEndTime_ = utils::TimeUtils::endOfDay(now);
        widgets::DatePicker::timestampToDate(displayStartTime_, displayStartDate_);
        widgets::DatePicker::timestampToDate(displayEndTime_, displayEndDate_);
        refreshEntries();
    }

    ImGui::SameLine();
    float width = ImGui::GetContentRegionAvail().x;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + width - 100);
    if (ImGui::Button("Add Entry", ImVec2(100, 0))) {
        startAdd();
    }

    ImGui::Separator();

    renderEntryList();

    if (showEditForm_) {
        renderEditForm();
    }

    if (showAddForm_) {
        renderAddForm();
    }

    ImGui::EndChild();
}

void EditorView::renderEntryList() {
    if (entries_.empty()) {
        ImGui::TextDisabled("No entries for this date range");
        return;
    }

    if (ImGui::BeginTable("Entries", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("YT Export", ImGuiTableColumnFlags_WidthFixed, 80);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        int64_t now = utils::TimeUtils::now();

        for (size_t i = 0; i < entries_.size(); i++) {
            const auto& fact = entries_[i];
            ImGui::TableNextRow();
            ImGui::PushID(static_cast<int>(i));

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", fact.activityName.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%s", utils::TimeUtils::formatDateTime(fact.startTime).c_str());

            ImGui::TableSetColumnIndex(2);
            if (fact.endTime.has_value()) {
                ImGui::Text("%s", utils::TimeUtils::formatDateTime(*fact.endTime).c_str());
            } else {
                ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "(ongoing)");
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", utils::TimeUtils::formatDuration(fact.getDuration(now)).c_str());

            ImGui::TableSetColumnIndex(4);
            if (fact.exportedToYoutrack) {
                ImGui::TextColored(ImVec4(0.0f, 0.8f, 0.0f, 1.0f), "%s", "\xE2\x9C\x93");  // ✓ checkmark (UTF-8)
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Exported to YouTrack");
                }
            } else {
                ImGui::TextDisabled("%s", "\xE2\x8A\x98");  // ⊘ empty set symbol (UTF-8)
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Not exported to YouTrack");
                }
            }

            ImGui::TableSetColumnIndex(5);
            if (ImGui::SmallButton("Edit")) {
                startEdit(fact);
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Delete")) {
                timeService_->deleteEntry(fact.id);
                refreshEntries();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void EditorView::renderEditForm() {
    ImGui::OpenPopup("Edit Entry");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 300));

    if (ImGui::BeginPopupModal("Edit Entry", &showEditForm_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Activity:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##editActivity", editActivityName_, sizeof(editActivityName_));

        ImGui::Spacing();
        ImGui::Text("Start:");
        widgets::DatePicker::renderWithCalendar("##editStartDate", editStartDate_);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##startHour", &editStartTime_[0], 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##startMin", &editStartTime_[1], 0);

        ImGui::Spacing();
        ImGui::Checkbox("Ongoing", &editIsOngoing_);

        if (!editIsOngoing_) {
            ImGui::Text("End:");
            widgets::DatePicker::renderWithCalendar("##editEndDate", editEndDate_);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40);
            ImGui::InputInt("##endHour", &editEndTime_[0], 0);
            ImGui::SameLine();
            ImGui::Text(":");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(40);
            ImGui::InputInt("##endMin", &editEndTime_[1], 0);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Save", ImVec2(100, 0))) {
            saveEdit();
            showEditForm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            showEditForm_ = false;
        }

        ImGui::EndPopup();
    }
}

void EditorView::renderAddForm() {
    ImGui::OpenPopup("Add Entry");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(400, 280));

    if (ImGui::BeginPopupModal("Add Entry", &showAddForm_, ImGuiWindowFlags_NoResize)) {
        ImGui::Text("Activity:");
        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("##addActivity", addActivityName_, sizeof(addActivityName_));

        ImGui::Spacing();
        ImGui::Text("Start:");
        widgets::DatePicker::renderWithCalendar("##addStartDate", addStartDate_);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##addStartHour", &addStartTime_[0], 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##addStartMin", &addStartTime_[1], 0);

        ImGui::Spacing();
        ImGui::Text("End:");
        widgets::DatePicker::renderWithCalendar("##addEndDate", addEndDate_);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##addEndHour", &addEndTime_[0], 0);
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        ImGui::InputInt("##addEndMin", &addEndTime_[1], 0);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Add", ImVec2(100, 0))) {
            saveAdd();
            showAddForm_ = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            showAddForm_ = false;
        }

        ImGui::EndPopup();
    }
}

void EditorView::refreshEntries() {
    entries_ = timeService_->getEntriesForRange(displayStartTime_, displayEndTime_);
}

void EditorView::startEdit(const models::Fact& fact) {
    editingFact_ = fact;
    strncpy(editActivityName_, fact.activityName.c_str(), sizeof(editActivityName_) - 1);
    setDateFromTimestamp(fact.startTime, editStartDate_, editStartTime_);

    editIsOngoing_ = !fact.endTime.has_value();
    if (fact.endTime.has_value()) {
        setDateFromTimestamp(*fact.endTime, editEndDate_, editEndTime_);
    } else {
        setDateFromTimestamp(utils::TimeUtils::now(), editEndDate_, editEndTime_);
    }

    showEditForm_ = true;
}

void EditorView::saveEdit() {
    if (!editingFact_.has_value()) return;

    models::Fact fact = *editingFact_;
    fact.startTime = getTimestampFromDate(editStartDate_, editStartTime_);

    if (editIsOngoing_) {
        fact.endTime = std::nullopt;
    } else {
        fact.endTime = getTimestampFromDate(editEndDate_, editEndTime_);
    }

    timeService_->updateEntry(fact, editActivityName_);
    refreshEntries();
}

void EditorView::deleteEntry() {
    if (editingFact_.has_value()) {
        timeService_->deleteEntry(editingFact_->id);
        refreshEntries();
    }
}

void EditorView::startAdd() {
    memset(addActivityName_, 0, sizeof(addActivityName_));
    int64_t now = utils::TimeUtils::now();
    setDateFromTimestamp(now - 3600, addStartDate_, addStartTime_);  // Default to 1 hour ago
    setDateFromTimestamp(now, addEndDate_, addEndTime_);
    showAddForm_ = true;
}

void EditorView::saveAdd() {
    if (strlen(addActivityName_) == 0) return;

    int64_t startTime = getTimestampFromDate(addStartDate_, addStartTime_);
    int64_t endTime = getTimestampFromDate(addEndDate_, addEndTime_);

    timeService_->addManualEntry(addActivityName_, startTime, endTime);
    refreshEntries();
}

void EditorView::setDateFromTimestamp(int64_t timestamp, int* date, int* time) {
    auto tm = utils::TimeUtils::toLocalTime(timestamp);
    date[0] = tm.tm_year + 1900;
    date[1] = tm.tm_mon + 1;
    date[2] = tm.tm_mday;
    time[0] = tm.tm_hour;
    time[1] = tm.tm_min;
}

int64_t EditorView::getTimestampFromDate(const int* date, const int* time) {
    return utils::TimeUtils::fromLocalTime(date[0], date[1], date[2], time[0], time[1], 0);
}

} // namespace timetracker::ui
