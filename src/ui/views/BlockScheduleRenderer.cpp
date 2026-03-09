#include "BlockScheduleRenderer.hpp"
#include "utils/TimeUtils.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <algorithm>

namespace timetracker::ui {

BlockScheduleRenderer::BlockScheduleRenderer(
    std::shared_ptr<services::SettingsService> settingsService,
    std::shared_ptr<services::TimeTrackingService> timeService)
    : settingsService_(std::move(settingsService))
    , timeService_(std::move(timeService)) {
}

std::vector<ScheduleBlock> BlockScheduleRenderer::computeBlocks(
    int64_t workdayStart, int64_t workdayEnd,
    const std::vector<models::Fact>& dayFacts) const {

    // Collect facts that fall (at least partially) within the workday window
    struct FactSlice {
        int64_t start;
        int64_t end;
        const models::Fact* fact;
    };
    std::vector<FactSlice> slices;

    for (const auto& f : dayFacts) {
        int64_t fStart = f.startTime;
        int64_t fEnd = f.endTime.value_or(utils::TimeUtils::now());

        // Only include facts that overlap the workday window
        if (fStart < workdayEnd && fEnd > workdayStart) {
            int64_t clampedStart = std::max(fStart, workdayStart);
            int64_t clampedEnd = std::min(fEnd, workdayEnd);
            if (clampedEnd - clampedStart >= 60 || !f.endTime.has_value()) {  // At least 1 minute (or ongoing)
                slices.push_back({clampedStart, clampedEnd, &f});
            }
        }
    }

    // Sort by start time
    std::sort(slices.begin(), slices.end(), [](const FactSlice& a, const FactSlice& b) {
        return a.start < b.start;
    });

    // Build block list: interleave gaps and facts
    std::vector<ScheduleBlock> blocks;
    int64_t cursor = workdayStart;

    for (const auto& slice : slices) {
        // Gap before this fact
        if (slice.start > cursor && (slice.start - cursor) >= 60) {
            blocks.push_back({cursor, slice.start, false, nullptr});
        }
        // Fact block
        blocks.push_back({slice.start, slice.end, true, slice.fact});
        cursor = std::max(cursor, slice.end);
    }

    // Trailing gap
    if (cursor < workdayEnd && (workdayEnd - cursor) >= 60) {
        blocks.push_back({cursor, workdayEnd, false, nullptr});
    }

    // If no blocks at all, the entire workday is empty
    if (blocks.empty()) {
        blocks.push_back({workdayStart, workdayEnd, false, nullptr});
    }

    return blocks;
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

    // Compute activity-driven blocks
    auto blocks = computeBlocks(workdayStart, workdayEnd, dayFacts);

    // Merge consecutive fact blocks with the same activity,
    // except KTalk meetings which are always shown individually.
    const std::string& ktalkActivity = L.get("KTalk Meeting");
    std::vector<ScheduleBlock> mergedBlocks;
    for (const auto& blk : blocks) {
        if (!mergedBlocks.empty() &&
                mergedBlocks.back().isFact && blk.isFact &&
                mergedBlocks.back().fact && blk.fact &&
                (mergedBlocks.back().fact->id == blk.fact->id ||
                        (mergedBlocks.back().fact->activityId == blk.fact->activityId &&
                         mergedBlocks.back().fact->activityName != ktalkActivity &&
                         mergedBlocks.back().fact->description == blk.fact->description)) &&
                mergedBlocks.back().endTime >= blk.startTime) {
            mergedBlocks.back().endTime = std::max(mergedBlocks.back().endTime, blk.endTime);
            mergedBlocks.back().mergedFacts.push_back(blk.fact);
        } else {
            mergedBlocks.push_back(blk);
            if (blk.fact) mergedBlocks.back().mergedFacts = {blk.fact};
        }
    }

    // Render table
    std::string tableId = "ScheduleTable_" + isoDate;
    if (ImGui::BeginTable(tableId.c_str(), 4,
            ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_SizingStretchProp)) {
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 110.0f);
        ImGui::TableSetupColumn("Activity", ImGuiTableColumnFlags_WidthStretch, 40.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 80.0f);
        ImGui::TableSetupColumn("Duration", ImGuiTableColumnFlags_WidthFixed, 80.0f);

        int64_t now = utils::TimeUtils::now();

        for (const auto& blk : mergedBlocks) {
            ImGui::TableNextRow();

            std::string blkStartStr = utils::TimeUtils::formatTime(blk.startTime);
            std::string blkEndStr = utils::TimeUtils::formatTime(blk.endTime);
            std::string timeRange = blkStartStr + " - " + blkEndStr;

            ImGui::PushID(static_cast<int>(blk.startTime));

            if (blk.isFact && blk.fact) {
                // Filled block — show fact info
                int64_t blkDuration = blk.endTime - blk.startTime;
                std::string durationStr = utils::TimeUtils::formatDuration(
                    blk.fact->isOngoing() ? (now - blk.startTime) : blkDuration);

                // Check overlap
                bool isOverlapping = false;
                if (blk.fact->endTime.has_value()) {
                    for (const auto& other : dayFacts) {
                        if (blk.fact->id != other.id && blk.fact->overlapsWith(other)) {
                            isOverlapping = true;
                            break;
                        }
                    }
                }

                if (isOverlapping) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
                }

                bool isMultiMerged = blk.mergedFacts.size() > 1;
                std::string popupId = "##MergedPicker_" + std::to_string(blk.startTime);

                // Time column (clickable to edit)
                ImGui::TableNextColumn();
                if (ImGui::Selectable(timeRange.c_str(), false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                    if (isMultiMerged) {
                        ImGui::OpenPopup(popupId.c_str());
                    } else {
                        action.type = ScheduleAction::EditFact;
                        action.factToEdit = blk.fact;
                    }
                }

                // Activity column
                ImGui::TableNextColumn();
                ImGui::Text("%s", blk.fact->activityName.c_str());
                if (isMultiMerged) {
                    ImGui::SameLine();
                    ImGui::TextDisabled("(%zu)", blk.mergedFacts.size());
                }

                // Description column
                ImGui::TableNextColumn();
                ImGui::Text("%s", blk.fact->description.c_str());

                // Duration column
                ImGui::TableNextColumn();
                float columnWidth = ImGui::GetContentRegionAvail().x;
                float textWidth = ImGui::CalcTextSize(durationStr.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + columnWidth - textWidth);
                ImGui::Text("%s", durationStr.c_str());

                if (isOverlapping) {
                    ImGui::PopStyleColor();
                }

                // Picker popup for merged blocks
                if (isMultiMerged && ImGui::BeginPopup(popupId.c_str())) {
                    ImGui::TextDisabled("%s", L.get("Entry to edit:"));
                    ImGui::Separator();
                    for (const auto* fp : blk.mergedFacts) {
                        std::string entryStart = utils::TimeUtils::formatTime(fp->startTime);
                        std::string entryEnd = fp->endTime.has_value()
                            ? utils::TimeUtils::formatTime(*fp->endTime)
                            : std::string(L.get("(ongoing)"));
                        std::string label = entryStart + " - " + entryEnd;
                        if (!fp->description.empty()) {
                            label += "  " + fp->description;
                        }
                        if (ImGui::Selectable(label.c_str())) {
                            action.type = ScheduleAction::EditFact;
                            action.factToEdit = fp;
                        }
                    }
                    ImGui::EndPopup();
                }
            } else {
                // Empty gap block — clickable to open Add activity modal
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 0.7f));

                ImGui::TableNextColumn();
                if (ImGui::Selectable(timeRange.c_str(), false,
                        ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                    action.type = ScheduleAction::CreateFact;
                    action.slotStart = blk.startTime;
                    action.slotEnd = blk.endTime;
                }

                ImGui::TableNextColumn();  // Activity
                ImGui::TableNextColumn();  // Description

                // Duration column — show gap duration dimmed
                ImGui::TableNextColumn();
                int64_t gapDuration = blk.endTime - blk.startTime;
                std::string gapDurStr = utils::TimeUtils::formatDuration(gapDuration);
                float colWidth = ImGui::GetContentRegionAvail().x;
                float txtWidth = ImGui::CalcTextSize(gapDurStr.c_str()).x;
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + colWidth - txtWidth);
                ImGui::Text("%s", gapDurStr.c_str());

                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    // Render facts outside workday window
    std::vector<const models::Fact*> outsideFacts;
    for (const auto& f : dayFacts) {
        int64_t fEnd = f.endTime.value_or(utils::TimeUtils::now());
        bool entirelyBefore = fEnd <= workdayStart;
        bool entirelyAfter = f.startTime >= workdayEnd;

        if (entirelyBefore || entirelyAfter) {
            outsideFacts.push_back(&f);
        } else if (f.startTime < workdayStart || fEnd > workdayEnd) {
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

} // namespace timetracker::ui
