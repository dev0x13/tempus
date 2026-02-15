#include "BlockScheduleRenderer.hpp"
#include "utils/TimeUtils.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <algorithm>
#include <cstring>

namespace timetracker::ui {

BlockScheduleRenderer::BlockScheduleRenderer(
    std::shared_ptr<services::SettingsService> settingsService,
    std::shared_ptr<services::TimeTrackingService> timeService)
    : settingsService_(std::move(settingsService))
    , timeService_(std::move(timeService))
    , autocomplete_(timeService_) {
    memset(quickAddName_, 0, sizeof(quickAddName_));
}

std::vector<ScheduleSegment> BlockScheduleRenderer::computeSegments(
    int64_t slotStart, int64_t slotEnd,
    const std::vector<models::Fact>& dayFacts) const {

    // Collect facts that overlap this slot
    struct FactSlice {
        int64_t start;
        int64_t end;
        const models::Fact* fact;
    };
    std::vector<FactSlice> slices;

    for (const auto& f : dayFacts) {
        int64_t fStart = f.startTime;
        int64_t fEnd = f.endTime.value_or(utils::TimeUtils::now());

        // Check overlap
        if (fStart < slotEnd && fEnd > slotStart) {
            int64_t clampedStart = std::max(fStart, slotStart);
            int64_t clampedEnd = std::min(fEnd, slotEnd);
            if (clampedEnd - clampedStart >= 60) {  // At least 1 minute
                slices.push_back({clampedStart, clampedEnd, &f});
            }
        }
    }

    // Sort by start time
    std::sort(slices.begin(), slices.end(), [](const FactSlice& a, const FactSlice& b) {
        return a.start < b.start;
    });

    // Build segments: interleave gaps and facts
    std::vector<ScheduleSegment> segments;
    int64_t cursor = slotStart;

    for (const auto& slice : slices) {
        // Gap before this fact
        if (slice.start > cursor && (slice.start - cursor) >= 60) {
            segments.push_back({cursor, slice.start, false, nullptr});
        }
        // Fact segment
        segments.push_back({slice.start, slice.end, true, slice.fact});
        cursor = std::max(cursor, slice.end);
    }

    // Trailing gap
    if (cursor < slotEnd && (slotEnd - cursor) >= 60) {
        segments.push_back({cursor, slotEnd, false, nullptr});
    }

    // If no segments at all, the entire slot is empty
    if (segments.empty()) {
        segments.push_back({slotStart, slotEnd, false, nullptr});
    }

    return segments;
}

ScheduleAction BlockScheduleRenderer::renderDay(
    const std::string& dateHeader,
    const std::string& isoDate,
    const std::vector<models::Fact>& dayFacts,
    int64_t dayStart) {

    auto& L = localization::L10n();
    ScheduleAction action;

    // Parse workday settings
    std::string startStr = settingsService_->getBlockScheduleWorkdayStart();
    std::string endStr = settingsService_->getBlockScheduleWorkdayEnd();
    int startHour = 9, startMin = 0, endHour = 18, endMin = 0;
    sscanf(startStr.c_str(), "%d:%d", &startHour, &startMin);
    sscanf(endStr.c_str(), "%d:%d", &endHour, &endMin);

    int periodMinutes = settingsService_->getBlockSchedulePeriod();

    // Compute workday timestamps for this day
    auto dayTm = utils::TimeUtils::toLocalTime(dayStart);
    int64_t workdayStart = utils::TimeUtils::fromLocalTime(
        dayTm.tm_year + 1900, dayTm.tm_mon + 1, dayTm.tm_mday, startHour, startMin, 0);
    int64_t workdayEnd = utils::TimeUtils::fromLocalTime(
        dayTm.tm_year + 1900, dayTm.tm_mon + 1, dayTm.tm_mday, endHour, endMin, 0);

    // Render date header
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "%s", dateHeader.c_str());
    ImGui::Spacing();

    ImGui::Indent(20.0f);

    // Generate slots and render the table
    std::string tableId = "ScheduleTable_" + isoDate;
    if (ImGui::BeginTable(tableId.c_str(), 4,
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 40.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 80.0f);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);

        int64_t now = utils::TimeUtils::now();

        // Generate and render each slot
        int64_t slotStart = workdayStart;
        int slotIndex = 0;
        while (slotStart < workdayEnd) {
            int64_t slotEnd = slotStart + periodMinutes * 60;
            if (slotEnd > workdayEnd) {
                slotEnd = workdayEnd;
            }

            auto segments = computeSegments(slotStart, slotEnd, dayFacts);

            for (const auto& seg : segments) {
                ImGui::TableNextRow();

                std::string segStartStr = utils::TimeUtils::formatTime(seg.startTime);
                std::string segEndStr = utils::TimeUtils::formatTime(seg.endTime);
                std::string timeRange = segStartStr + " - " + segEndStr;

                ImGui::PushID(static_cast<int>(seg.startTime ^ (slotIndex * 1000)));

                if (seg.isFact && seg.fact) {
                    // Filled segment - show fact info
                    int64_t segDuration = seg.endTime - seg.startTime;
                    std::string durationStr = utils::TimeUtils::formatDuration(
                        seg.fact->isOngoing() ? (now - seg.startTime) : segDuration);

                    // Check overlap
                    bool isOverlapping = false;
                    if (seg.fact->endTime.has_value()) {
                        for (const auto& other : dayFacts) {
                            if (seg.fact->id != other.id && seg.fact->overlapsWith(other)) {
                                isOverlapping = true;
                                break;
                            }
                        }
                    }

                    if (isOverlapping) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                    }

                    // Time column (clickable to edit)
                    ImGui::TableNextColumn();
                    if (ImGui::Selectable(timeRange.c_str(), false,
                            ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                        action.type = ScheduleAction::EditFact;
                        action.factToEdit = seg.fact;
                    }

                    // Activity column
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", seg.fact->activityName.c_str());

                    // Description column
                    ImGui::TableNextColumn();
                    ImGui::Text("%s", seg.fact->description.c_str());

                    // Duration column
                    ImGui::TableNextColumn();
                    float columnWidth = ImGui::GetContentRegionAvail().x;
                    float textWidth = ImGui::CalcTextSize(durationStr.c_str()).x;
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columnWidth - textWidth);
                    ImGui::Text("%s", durationStr.c_str());

                    if (isOverlapping) {
                        ImGui::PopStyleColor();
                    }
                } else {
                    // Empty segment - clickable to quick-add
                    bool isActiveQuickAdd = quickAddActive_ &&
                        quickAddStart_ == seg.startTime && quickAddEnd_ == seg.endTime;

                    if (isActiveQuickAdd) {
                        // Render inline input
                        ImGui::TableNextColumn();
                        ImGui::TextDisabled("%s", timeRange.c_str());

                        ImGui::TableNextColumn();
                        ImGui::SetNextItemWidth(-1);

                        if (!quickAddFocusSet_) {
                            ImGui::SetKeyboardFocusHere();
                            quickAddFocusSet_ = true;
                        }

                        bool confirmed = autocomplete_.render("##quickAddInline", quickAddName_, sizeof(quickAddName_));
                        if (confirmed && strlen(quickAddName_) > 0) {
                            timeService_->addManualEntry(quickAddName_, quickAddStart_, quickAddEnd_);
                            cancelQuickAdd();
                            // Return no action - the view will refresh entries
                        }

                        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                            cancelQuickAdd();
                        }

                        ImGui::TableNextColumn();  // Description
                        ImGui::TableNextColumn();  // Duration
                    } else {
                        // Empty row - clickable
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));

                        ImGui::TableNextColumn();
                        if (ImGui::Selectable(timeRange.c_str(), false,
                                ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                            startQuickAdd(seg.startTime, seg.endTime);
                        }

                        ImGui::TableNextColumn();  // Activity
                        ImGui::TableNextColumn();  // Description

                        // Duration column - show slot duration dimmed
                        ImGui::TableNextColumn();
                        int64_t gapDuration = seg.endTime - seg.startTime;
                        std::string gapDurStr = utils::TimeUtils::formatDuration(gapDuration);
                        float colWidth = ImGui::GetContentRegionAvail().x;
                        float txtWidth = ImGui::CalcTextSize(gapDurStr.c_str()).x;
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colWidth - txtWidth);
                        ImGui::Text("%s", gapDurStr.c_str());

                        ImGui::PopStyleColor();
                    }
                }

                ImGui::PopID();
            }

            slotStart = slotEnd;
            slotIndex++;
        }

        ImGui::EndTable();
    }

    // Render facts outside workday window
    std::vector<const models::Fact*> outsideFacts;
    for (const auto& f : dayFacts) {
        int64_t fEnd = f.endTime.value_or(utils::TimeUtils::now());
        // Fact is entirely outside if it ends before workday start or starts after workday end
        bool entirelyBefore = fEnd <= workdayStart;
        bool entirelyAfter = f.startTime >= workdayEnd;

        if (entirelyBefore || entirelyAfter) {
            outsideFacts.push_back(&f);
        } else if (f.startTime < workdayStart || fEnd > workdayEnd) {
            // Partially outside - the part outside the workday window
            // We show it in the outside section only if it extends before workday start or after end
            // The in-workday portion is already handled by slot rendering above
            // We only show facts that have a meaningful portion outside
            if (f.startTime < workdayStart && (workdayStart - f.startTime) >= 60) {
                outsideFacts.push_back(&f);
            } else if (fEnd > workdayEnd && (fEnd - workdayEnd) >= 60) {
                outsideFacts.push_back(&f);
            }
        }
    }

    if (!outsideFacts.empty()) {
        ImGui::Spacing();
        ImGui::TextDisabled("%s", L.get("Outside workday"));
        std::string outsideTableId = "OutsideTable_" + isoDate;
        if (ImGui::BeginTable(outsideTableId.c_str(), 4,
                ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 110.0f);
            ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 40.0f);
            ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 80.0f);
            ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);

            int64_t now = utils::TimeUtils::now();
            for (const auto* fp : outsideFacts) {
                ImGui::TableNextRow();
                ImGui::PushID(static_cast<int>(fp->id + 50000));

                std::string startT = utils::TimeUtils::formatTime(fp->startTime);
                std::string endT = fp->endTime.has_value()
                    ? utils::TimeUtils::formatTime(*fp->endTime)
                    : std::string(L.get("(ongoing)"));
                std::string range = startT + " - " + endT;

                ImGui::TableNextColumn();
                if (ImGui::Selectable(range.c_str(), false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                    action.type = ScheduleAction::EditFact;
                    action.factToEdit = fp;
                }

                ImGui::TableNextColumn();
                ImGui::Text("%s", fp->activityName.c_str());

                ImGui::TableNextColumn();
                ImGui::Text("%s", fp->description.c_str());

                ImGui::TableNextColumn();
                std::string dur = utils::TimeUtils::formatDuration(fp->getDuration(now));
                float cw = ImGui::GetContentRegionAvail().x;
                float tw = ImGui::CalcTextSize(dur.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + cw - tw);
                ImGui::Text("%s", dur.c_str());

                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }

    ImGui::Unindent(20.0f);
    ImGui::Spacing();

    return action;
}

bool BlockScheduleRenderer::renderQuickAdd() {
    // Quick add is rendered inline within renderDay
    return false;
}

void BlockScheduleRenderer::cancelQuickAdd() {
    quickAddActive_ = false;
    quickAddStart_ = 0;
    quickAddEnd_ = 0;
    memset(quickAddName_, 0, sizeof(quickAddName_));
    autocomplete_.clear();
    quickAddFocusSet_ = false;
}

void BlockScheduleRenderer::startQuickAdd(int64_t start, int64_t end) {
    quickAddActive_ = true;
    quickAddStart_ = start;
    quickAddEnd_ = end;
    memset(quickAddName_, 0, sizeof(quickAddName_));
    autocomplete_.clear();
    quickAddFocusSet_ = false;
}

} // namespace timetracker::ui
