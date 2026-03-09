#pragma once

#include "services/TimeTrackingService.hpp"
#include "models/Activity.hpp"
#include "imgui.h"
#include <memory>
#include <string>
#include <vector>

namespace timetracker::ui::widgets {

class ActivityAutocomplete {
public:
    explicit ActivityAutocomplete(std::shared_ptr<services::TimeTrackingService> timeService);
    ~ActivityAutocomplete() = default;

    // Render the input field. Call inside parent window.
    // Returns true if an activity was selected.
    bool render(const char* label, char* buffer, size_t bufferSize);

    // Render the suggestion dropdown as a top-level window.
    // For regular windows: call AFTER parent window's ImGui::End().
    // For modal popups: call just before ImGui::EndPopup().
    void renderDropdown();

    // True when suggestions are visible; use to set NoBringToFrontOnFocus on parent.
    bool hasSuggestions() const { return showSuggestions_; }

    const std::string& getSelectedActivity() const { return selectedActivity_; }

    void clear();

    // Called by InputText callback — do not call directly.
    int handleCallback(ImGuiInputTextCallbackData* data);

private:
    static int inputCallback(ImGuiInputTextCallbackData* data);

    std::shared_ptr<services::TimeTrackingService> timeService_;

    std::vector<models::Activity> suggestions_;
    std::string selectedActivity_;
    bool showSuggestions_{false};
    int activeIdx_{-1};
    int clickedIdx_{-1};
    bool selectionChanged_{false};
    bool pendingSelection_{false};
    bool dropdownWasOpen_{false};
    bool needsRefocus_{false};
    std::string suppressText_;
    ImVec2 dropdownPos_{0, 0};
    float dropdownWidth_{300.0f};
};

} // namespace timetracker::ui::widgets
