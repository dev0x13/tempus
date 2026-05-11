#include "ActivityAutocomplete.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstring>

#include "localization/LocalizationManager.hpp"

namespace timetracker::ui::widgets {

ActivityAutocomplete::ActivityAutocomplete(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {}

int ActivityAutocomplete::inputCallback(ImGuiInputTextCallbackData* data) {
    return static_cast<ActivityAutocomplete*>(data->UserData)->handleCallback(data);
}

int ActivityAutocomplete::handleCallback(ImGuiInputTextCallbackData* data) {
    switch (data->EventFlag) {
        case ImGuiInputTextFlags_CallbackCompletion:
            // Tab: complete with the active suggestion
            if (showSuggestions_ && activeIdx_ >= 0 && activeIdx_ < (int)suggestions_.size()) {
                const auto& name = suggestions_[activeIdx_].name;
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, name.c_str());
            }
            showSuggestions_ = false;
            activeIdx_ = -1;
            clickedIdx_ = -1;
            break;

        case ImGuiInputTextFlags_CallbackHistory:
            // Up/Down arrows: navigate suggestions
            if (!suggestions_.empty()) {
                showSuggestions_ = true;
                if (data->EventKey == ImGuiKey_UpArrow) {
                    if (activeIdx_ > 0) {
                        activeIdx_--;
                        selectionChanged_ = true;
                    } else if (activeIdx_ == 0) {
                        activeIdx_ = -1;
                        selectionChanged_ = true;
                    }
                } else if (data->EventKey == ImGuiKey_DownArrow) {
                    if (activeIdx_ < (int)suggestions_.size() - 1) {
                        activeIdx_++;
                        selectionChanged_ = true;
                    }
                }
            }
            break;

        case ImGuiInputTextFlags_CallbackAlways:
            // A mouse click in the dropdown set clickedIdx_; apply it to the buffer now.
            if (clickedIdx_ >= 0 && clickedIdx_ < (int)suggestions_.size()) {
                const auto& name = suggestions_[clickedIdx_].name;
                data->DeleteChars(0, data->BufTextLen);
                data->InsertChars(0, name.c_str());
                selectedActivity_ = name;
                pendingSelection_ = true;
                showSuggestions_ = false;
                activeIdx_ = -1;
                clickedIdx_ = -1;
            }
            break;
    }
    return 0;
}

bool ActivityAutocomplete::render(const char* label, char* buffer, size_t bufferSize) {
    auto& L = localization::L10n();

    bool selected = false;
    pendingSelection_ = false;

    ImGui::PushID(label);

    ImGui::SetNextItemWidth(350);

    // Reclaim keyboard focus after a mouse click on a dropdown item.
    if (needsRefocus_) {
        ImGui::SetKeyboardFocusHere();
        needsRefocus_ = false;
    }

    ImGuiInputTextFlags flags =
        ImGuiInputTextFlags_EnterReturnsTrue   |
        ImGuiInputTextFlags_CallbackAlways     |
        ImGuiInputTextFlags_CallbackCompletion |
        ImGuiInputTextFlags_CallbackHistory;

    bool enterPressed = ImGui::InputTextWithHint(
        "##input", L.get("Activity name or YouTrack issue ID"),
        buffer, bufferSize,
        flags, inputCallback, this);

    bool inputFocused = ImGui::IsItemFocused();

    // Handle Enter key — must run before suggestion update so that
    // 'selected' is set and we can skip reopening the dropdown.
    if (enterPressed) {
        if (showSuggestions_ && activeIdx_ >= 0 && activeIdx_ < (int)suggestions_.size()) {
            selectedActivity_ = suggestions_[activeIdx_].name;
            strncpy(buffer, selectedActivity_.c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            selected = true;
        } else if (strlen(buffer) > 0) {
            selectedActivity_ = buffer;
            selected = true;
        }
        showSuggestions_ = false;
        activeIdx_ = -1;
    }

    // Propagate a click-based selection
    if (pendingSelection_) {
        selected = true;
    }

    // After a selection, remember the text so we suppress the dropdown
    // until the user actually edits the buffer.
    if (selected) {
        suppressText_ = buffer;
    } else if (!suppressText_.empty() && suppressText_ != buffer) {
        suppressText_.clear();
    }

    // Update suggestions based on current buffer content.
    if (!suppressText_.empty()) {
        showSuggestions_ = false;
    } else if (strlen(buffer) > 0) {
            auto newSuggestions = timeService_->searchActivities(buffer, 5);
            bool changed = (newSuggestions.size() != suggestions_.size());
            if (!changed && !newSuggestions.empty() && !suggestions_.empty()) {
                changed = (newSuggestions[0].id != suggestions_[0].id);
            }
            if (changed) {
                suggestions_ = newSuggestions;
                activeIdx_ = -1;
                selectionChanged_ = false;
            }
            showSuggestions_ = !suggestions_.empty();
        } else {
            showSuggestions_ = false;
            suggestions_.clear();
            activeIdx_ = -1;
        }

    // Store position for renderDropdown()
    dropdownPos_ = ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y);
    dropdownWidth_ = ImGui::GetItemRectSize().x;

    // Escape closes the dropdown (dialog Escape is handled by the caller)
    if (showSuggestions_ && inputFocused && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        showSuggestions_ = false;
        activeIdx_ = -1;
    }

    if (label[0] != '#') {
        ImGui::SameLine();
        ImGui::Text("%s", label);
    }

    ImGui::PopID();
    return selected;
}

void ActivityAutocomplete::renderDropdown() {
    if (!showSuggestions_ || suggestions_.empty()) {
        dropdownWasOpen_ = false;
        return;
    }

    dropdownWasOpen_ = true;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar        |
        ImGuiWindowFlags_NoResize          |
        ImGuiWindowFlags_NoMove            |
        ImGuiWindowFlags_NoScrollbar       |
        ImGuiWindowFlags_NoSavedSettings   |
        ImGuiWindowFlags_NoFocusOnAppearing;

    float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    float padding = ImGui::GetStyle().WindowPadding.y * 2.0f;
    float dropdownHeight = itemHeight * static_cast<float>(suggestions_.size()) + padding;

    ImGui::SetNextWindowPos(dropdownPos_);
    ImGui::SetNextWindowSize(ImVec2(dropdownWidth_, dropdownHeight));
    ImGui::SetNextWindowBgAlpha(1.0f);

    if (ImGui::Begin("##autocomplete_dropdown", nullptr, flags)) {
        // Force the dropdown to the front of the display list without
        // touching keyboard/nav focus.  This uses imgui_internal.h.
        ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());

        ImGui::PushAllowKeyboardFocus(false);

        for (int i = 0; i < (int)suggestions_.size(); i++) {
            bool isActive = (i == activeIdx_);

            ImGui::PushID(i);
            if (ImGui::Selectable(suggestions_[i].name.c_str(), isActive)) {
                clickedIdx_ = i;
                needsRefocus_ = true;
            }
            ImGui::PopID();

            if (isActive && selectionChanged_) {
                ImGui::SetScrollHereY();
                selectionChanged_ = false;
            }
        }

        ImGui::PopAllowKeyboardFocus();
    }
    ImGui::End();

    ImGui::PopStyleVar();
}

void ActivityAutocomplete::clear() {
    selectedActivity_.clear();
    suggestions_.clear();
    showSuggestions_ = false;
    activeIdx_ = -1;
    clickedIdx_ = -1;
    pendingSelection_ = false;
    selectionChanged_ = false;
    dropdownWasOpen_ = false;
    needsRefocus_ = false;
    suppressText_.clear();
}

} // namespace timetracker::ui::widgets
