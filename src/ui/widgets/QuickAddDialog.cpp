#include "QuickAddDialog.hpp"
#include "localization/LocalizationManager.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui::widgets {

QuickAddDialog::QuickAddDialog(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)), autocomplete_(timeService_) {
    memset(activityInput_, 0, sizeof(activityInput_));
}

void QuickAddDialog::render() {
    if (!visible_) return;

    auto& L = localization::L10n();

    // Position dialog near cursor (or center of screen)
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(0, 0), ImGuiCond_Appearing);

    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
    // Prevent the dialog from jumping in front of the suggestion dropdown when re-focused
    if (autocomplete_.hasSuggestions())
        windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus;

    if (ImGui::Begin(L.get("Quick add activity"), &visible_, windowFlags)) {
        ImGui::SetNextItemWidth(-1);

        // Auto-focus input when dialog appears
        if (ImGui::IsWindowAppearing()) {
            ImGui::SetKeyboardFocusHere();
        }

        bool enterPressed = autocomplete_.render("##activityInput", activityInput_, sizeof(activityInput_));
        // Fix for GNOME (#36)
        ImGui::SetItemKeyOwner(ImGuiMod_Alt);

        ImGui::Spacing();

        // Start button
        bool shouldStart = false;
        if (ImGui::Button(L.get("Start tracking"), ImVec2(120, 0)) || enterPressed) {
            shouldStart = true;
        }

        ImGui::SameLine();
        if (ImGui::Button(L.get("Cancel"), ImVec2(80, 0))) {
            hide();
        }

        if (shouldStart) {
            std::string activity = autocomplete_.getSelectedActivity();
            if (activity.empty() && strlen(activityInput_) > 0) {
                activity = activityInput_;
            }
            if (activity.empty()) {
                nameError_ = true;
            } else {
                nameError_ = false;

                // Start tracking
                timeService_->startTracking(activity);

                // Notify callback
                if (onTrackingStarted_) {
                    onTrackingStarted_();
                }

                // Clear and hide
                memset(activityInput_, 0, sizeof(activityInput_));
                hide();
            }
        }

        if (nameError_) {
            ImGui::Spacing();
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s", L.get("Activity name is required"));
        }
    }
    ImGui::End();

    // Render the suggestion dropdown as a top-level window so it overlays everything
    // and receives mouse clicks without stealing keyboard focus from the input.
    autocomplete_.renderDropdown();

    // Escape closes the dialog only when the dropdown is not open; when the dropdown
    // is open, Escape is handled inside ActivityAutocomplete::render() to close it first.
    if (visible_ && !autocomplete_.hasSuggestions() && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        hide();
    }
}

void QuickAddDialog::show() {
    visible_ = true;
    nameError_ = false;
    memset(activityInput_, 0, sizeof(activityInput_));
    autocomplete_.clear();
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

} // namespace timetracker::ui::widgets
