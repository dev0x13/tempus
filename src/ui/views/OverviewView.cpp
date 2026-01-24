#include "OverviewView.hpp"
#include "utils/TimeUtils.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui {

OverviewView::OverviewView(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {
    memset(activityInput_, 0, sizeof(activityInput_));
}

void OverviewView::render() {
    // Refresh recent entries periodically (every 5 seconds)
    int64_t now = utils::TimeUtils::now();
    if (now - lastRefresh_ > 5) {
        refreshRecentEntries();
    }

    ImGui::BeginChild("OverviewContent", ImVec2(0, 0), false);

    renderCurrentTracking();
    ImGui::Separator();
    renderQuickStart();
    ImGui::Separator();
    renderRecentEntries();

    ImGui::EndChild();
}

void OverviewView::renderCurrentTracking() {
    ImGui::Text("Current Tracking");
    ImGui::Spacing();

    auto current = timeService_->getCurrentTracking();
    if (current.has_value()) {
        int64_t now = utils::TimeUtils::now();
        int64_t duration = current->getDuration(now);

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 0.4f, 1.0f));
        ImGui::Text("%s", current->activityName.c_str());
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", utils::TimeUtils::formatDuration(duration).c_str());

        ImGui::Spacing();

        if (ImGui::Button("Stop Tracking", ImVec2(150, 30))) {
            timeService_->stopTracking();
            refreshRecentEntries();
        }
    } else {
        ImGui::TextDisabled("Not tracking");
    }

    ImGui::Spacing();
}

void OverviewView::renderQuickStart() {
    ImGui::Text("Quick Start");
    ImGui::Spacing();

    // Activity input with autocomplete
    ImGui::SetNextItemWidth(300);
    bool startPressed = ImGui::InputTextWithHint(
        "##activity", "Enter activity name...",
        activityInput_, sizeof(activityInput_),
        ImGuiInputTextFlags_EnterReturnsTrue);

    // Autocomplete dropdown
    if (strlen(activityInput_) > 0) {
        auto suggestions = timeService_->searchActivities(activityInput_, 5);
        if (!suggestions.empty()) {
            ImGui::SetNextWindowPos(
                ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
            ImGui::SetNextWindowSize(ImVec2(300, 0));

            if (ImGui::BeginPopup("##autocomplete", ImGuiWindowFlags_NoFocusOnAppearing)) {
                for (const auto& activity : suggestions) {
                    if (ImGui::Selectable(activity.name.c_str())) {
                        strncpy(activityInput_, activity.name.c_str(), sizeof(activityInput_) - 1);
                        startPressed = true;
                    }
                }
                ImGui::EndPopup();
            }

            // Open popup if we have suggestions
            if (ImGui::IsItemActive()) {
                ImGui::OpenPopup("##autocomplete");
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Start", ImVec2(80, 0)) || startPressed) {
        if (strlen(activityInput_) > 0) {
            timeService_->startTracking(activityInput_);
            memset(activityInput_, 0, sizeof(activityInput_));
            refreshRecentEntries();
        }
    }

    ImGui::Spacing();
}

void OverviewView::renderRecentEntries() {
    ImGui::Text("Recent Entries");
    ImGui::Spacing();

    if (recentEntries_.empty()) {
        ImGui::TextDisabled("No entries yet");
        return;
    }

    // Table of recent entries
    if (ImGui::BeginTable("RecentEntries", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Start", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("End", ImGuiTableColumnFlags_WidthFixed, 120);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableHeadersRow();

        int64_t now = utils::TimeUtils::now();

        for (const auto& fact : recentEntries_) {
            ImGui::TableNextRow();

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
        }

        ImGui::EndTable();
    }
}

void OverviewView::refreshRecentEntries() {
    recentEntries_ = timeService_->getRecentEntries(10);
    lastRefresh_ = utils::TimeUtils::now();
}

} // namespace timetracker::ui
