#pragma once

#include "services/TimeTrackingService.hpp"
#include "models/Activity.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace timetracker::ui::widgets {

class ActivityAutocomplete {
public:
    explicit ActivityAutocomplete(std::shared_ptr<services::TimeTrackingService> timeService);
    ~ActivityAutocomplete() = default;

    // Render the autocomplete widget
    // Returns true if an activity was selected (either by pressing Enter or clicking suggestion)
    bool render(const char* label, char* buffer, size_t bufferSize);

    // Get the selected activity name (valid after render returns true)
    const std::string& getSelectedActivity() const { return selectedActivity_; }

    // Clear the selection
    void clear();

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;

    std::vector<models::Activity> suggestions_;
    std::string selectedActivity_;
    bool showSuggestions_{false};
    int selectedSuggestion_{-1};
};

} // namespace timetracker::ui::widgets
