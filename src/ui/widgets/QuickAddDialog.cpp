#include "QuickAddDialog.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui::widgets {

QuickAddDialog::QuickAddDialog(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {
    memset(activityInput_, 0, sizeof(activityInput_));
}

void QuickAddDialog::render() {
    if (!visible_) return;

    auto& L = localization::L10n();

    // Position dialog near cursor (or center of screen)
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 120), ImGuiCond_Appearing);

    if (ImGui::Begin(L.QuickAdd.Title(), &visible_, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
        ImGui::Text("%s", L.QuickAdd.ActivityName());
        ImGui::SetNextItemWidth(-1);

        // Auto-focus input when dialog appears
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        bool enterPressed = ImGui::InputText("##activityInput", activityInput_, sizeof(activityInput_),
                                               ImGuiInputTextFlags_EnterReturnsTrue);

        ImGui::Spacing();

        // Start button
        bool shouldStart = false;
        if (ImGui::Button(L.QuickAdd.StartTracking(), ImVec2(120, 0)) || enterPressed) {
            shouldStart = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(L.Common.Cancel(), ImVec2(80, 0))) {
            hide();
        }

        if (shouldStart && strlen(activityInput_) > 0) {
            // Start tracking
            timeService_->startTracking(activityInput_);

            // Notify callback
            if (onTrackingStarted_) {
                onTrackingStarted_();
            }

            // Clear and hide
            memset(activityInput_, 0, sizeof(activityInput_));
            hide();
        }
    }
    ImGui::End();

    // Allow closing with Escape
    if (visible_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        hide();
    }
}

void QuickAddDialog::show() {
    visible_ = true;
    memset(activityInput_, 0, sizeof(activityInput_));
}

void QuickAddDialog::hide() {
    visible_ = false;
    // Notify that dialog is closing
    if (onDialogClosed_) {
        onDialogClosed_();
    }
}

void QuickAddDialog::setOnTrackingStarted(std::function<void()> callback) {
    onTrackingStarted_ = std::move(callback);
}

void QuickAddDialog::setOnDialogClosed(std::function<void()> callback) {
    onDialogClosed_ = std::move(callback);
}

void QuickAddDialog::renderAutocomplete() {
    // TODO: Implement autocomplete based on recent activities
}

} // namespace timetracker::ui::widgets
