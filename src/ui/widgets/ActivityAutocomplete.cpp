#include "ActivityAutocomplete.hpp"
#include "imgui.h"
#include <cstring>

namespace timetracker::ui::widgets {

ActivityAutocomplete::ActivityAutocomplete(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {}

bool ActivityAutocomplete::render(const char* label, char* buffer, size_t bufferSize) {
    bool selected = false;

    ImGui::PushID(label);

    // Text input
    ImGui::SetNextItemWidth(300);
    bool enterPressed = ImGui::InputTextWithHint(
        "##input", "Enter activity name...",
        buffer, bufferSize,
        ImGuiInputTextFlags_EnterReturnsTrue);

    bool inputActive = ImGui::IsItemActive();
    bool inputFocused = ImGui::IsItemFocused();

    // Update suggestions when typing
    if ((inputActive || inputFocused) && strlen(buffer) > 0) {
        auto newSuggestions = timeService_->searchActivities(buffer, 5);
        // Only update if suggestions changed to avoid resetting selection
        bool suggestionsChanged = (newSuggestions.size() != suggestions_.size());
        if (!suggestionsChanged && !newSuggestions.empty() && !suggestions_.empty()) {
            suggestionsChanged = (newSuggestions[0].id != suggestions_[0].id);
        }
        if (suggestionsChanged) {
            suggestions_ = newSuggestions;
            selectedSuggestion_ = -1; // Reset selection when suggestions change
        }
        showSuggestions_ = !suggestions_.empty();
    } else if (!inputFocused) {
        showSuggestions_ = false;
        selectedSuggestion_ = -1;
    }

    // Handle Enter key
    if (enterPressed) {
        // If a suggestion is selected via arrow keys, use that
        if (showSuggestions_ && selectedSuggestion_ >= 0 &&
            selectedSuggestion_ < static_cast<int>(suggestions_.size())) {
            selectedActivity_ = suggestions_[selectedSuggestion_].name;
            selected = true;
            showSuggestions_ = false;
            selectedSuggestion_ = -1;
        } else if (strlen(buffer) > 0) {
            // Otherwise use the typed text
            selectedActivity_ = buffer;
            selected = true;
            showSuggestions_ = false;
        }
    }

    // Handle keyboard navigation when input is focused and suggestions are showing
    bool navigationKeyPressed = false;
    if (showSuggestions_ && inputFocused && !suggestions_.empty()) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            if (selectedSuggestion_ < 0) {
                selectedSuggestion_ = 0;
            } else {
                selectedSuggestion_ = (selectedSuggestion_ + 1) % static_cast<int>(suggestions_.size());
            }
            navigationKeyPressed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            if (selectedSuggestion_ < 0) {
                selectedSuggestion_ = static_cast<int>(suggestions_.size()) - 1;
            } else {
                selectedSuggestion_--;
                if (selectedSuggestion_ < 0) {
                    selectedSuggestion_ = static_cast<int>(suggestions_.size()) - 1;
                }
            }
            navigationKeyPressed = true;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Tab) && selectedSuggestion_ >= 0 &&
                 selectedSuggestion_ < static_cast<int>(suggestions_.size())) {
            strncpy(buffer, suggestions_[selectedSuggestion_].name.c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
            showSuggestions_ = false;
            selectedSuggestion_ = -1;
        }
        else if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showSuggestions_ = false;
            selectedSuggestion_ = -1;
        }
    }

    // Re-focus the input if we handled a navigation key
    if (navigationKeyPressed) {
        ImGui::SetKeyboardFocusHere(-1);  // -1 = previous item (the input we just rendered)
    }

    // Render suggestions dropdown
    if (showSuggestions_) {
        // Open popup on the frame when suggestions become available
        if (inputActive) {
            ImGui::OpenPopup("##autocomplete_popup");
        }

        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(300, 0));

        if (ImGui::BeginPopup("##autocomplete_popup", ImGuiWindowFlags_NoFocusOnAppearing)) {
            for (size_t i = 0; i < suggestions_.size(); i++) {
                bool isSelected = (static_cast<int>(i) == selectedSuggestion_);

                // Highlight the selected item with a different color
                if (isSelected) {
                    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
                }

                if (ImGui::Selectable(suggestions_[i].name.c_str(), isSelected)) {
                    strncpy(buffer, suggestions_[i].name.c_str(), bufferSize - 1);
                    buffer[bufferSize - 1] = '\0';
                    selectedActivity_ = suggestions_[i].name;
                    selected = true;
                    showSuggestions_ = false;
                    selectedSuggestion_ = -1;
                    ImGui::CloseCurrentPopup();
                }

                if (isSelected) {
                    ImGui::PopStyleColor();
                }
            }
            ImGui::EndPopup();
        } else if (!inputFocused) {
            // Popup was closed
            showSuggestions_ = false;
            selectedSuggestion_ = -1;
        }
    }

    if (label[0] != '#') {
        ImGui::SameLine();
        ImGui::Text("%s", label);
    }

    ImGui::PopID();
    return selected;
}

void ActivityAutocomplete::clear() {
    selectedActivity_.clear();
    suggestions_.clear();
    showSuggestions_ = false;
    selectedSuggestion_ = -1;
}

} // namespace timetracker::ui::widgets
