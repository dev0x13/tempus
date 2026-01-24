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
    if (inputActive && strlen(buffer) > 0) {
        suggestions_ = timeService_->searchActivities(buffer, 5);
        showSuggestions_ = !suggestions_.empty();
    } else if (!inputFocused) {
        showSuggestions_ = false;
    }

    // Handle Enter key
    if (enterPressed && strlen(buffer) > 0) {
        selectedActivity_ = buffer;
        selected = true;
        showSuggestions_ = false;
    }

    // Render suggestions dropdown
    if (showSuggestions_) {
        ImGui::SetNextWindowPos(
            ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
        ImGui::SetNextWindowSize(ImVec2(300, 0));

        ImGuiWindowFlags flags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_AlwaysAutoResize;

        if (ImGui::Begin("##suggestions", nullptr, flags)) {
            for (size_t i = 0; i < suggestions_.size(); i++) {
                bool isSelected = (static_cast<int>(i) == selectedSuggestion_);
                if (ImGui::Selectable(suggestions_[i].name.c_str(), isSelected)) {
                    strncpy(buffer, suggestions_[i].name.c_str(), bufferSize - 1);
                    buffer[bufferSize - 1] = '\0';
                    selectedActivity_ = suggestions_[i].name;
                    selected = true;
                    showSuggestions_ = false;
                }
            }
        }
        ImGui::End();
    }

    // Handle keyboard navigation in suggestions
    if (showSuggestions_ && inputFocused) {
        if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
            selectedSuggestion_ = (selectedSuggestion_ + 1) % static_cast<int>(suggestions_.size());
        }
        if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
            selectedSuggestion_--;
            if (selectedSuggestion_ < 0) {
                selectedSuggestion_ = static_cast<int>(suggestions_.size()) - 1;
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Tab) && selectedSuggestion_ >= 0) {
            strncpy(buffer, suggestions_[selectedSuggestion_].name.c_str(), bufferSize - 1);
            buffer[bufferSize - 1] = '\0';
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            showSuggestions_ = false;
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
