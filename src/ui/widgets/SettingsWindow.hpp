#pragma once

#include "services/SettingsService.hpp"
#include <memory>
#include <string>
#include <vector>

namespace timetracker::ui::widgets {

struct ActivityAlias {
    char activityName[128]{};
    char issueId[64]{};
    bool markedForDeletion{false};
};

class SettingsWindow {
public:
    explicit SettingsWindow(std::shared_ptr<services::SettingsService> settingsService);
    ~SettingsWindow() = default;

    // Render the window (call this every frame if visible)
    void render();

    // Show/hide the window
    void show();
    void hide();
    bool isVisible() const { return visible_; }

private:
    std::shared_ptr<services::SettingsService> settingsService_;
    bool visible_{false};

    // Form fields
    char youtrackUrl_[256]{};
    char youtrackToken_[256]{};
    std::vector<ActivityAlias> aliases_;
    bool ktalkIncludeUnplanned_{true};

    // Validation state
    bool hasValidationError_{false};
    std::string validationMessage_;

    // Load current settings into form fields
    void loadSettings();

    // Save form fields to settings
    void saveSettings();

    // Validate form inputs
    bool validateInputs();

    // Render sections
    void renderYouTrackSettings();
    void renderActivityAliases();
    void renderKTalkSettings();
};

} // namespace timetracker::ui::widgets
