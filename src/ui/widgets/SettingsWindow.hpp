#pragma once

#include "services/SettingsService.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>

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

    // Set callback for Export Log button
    void setExportLogCallback(std::function<void()> callback) { exportLogCallback_ = std::move(callback); }

private:
    std::shared_ptr<services::SettingsService> settingsService_;
    bool visible_{false};

    // Form fields
    char youtrackUrl_[256]{};
    char youtrackToken_[256]{};
    std::vector<ActivityAlias> aliases_;
    bool ktalkIncludeUnplanned_{true};
    int ktalkSnapInterval_{0};
    int selectedLanguage_{1};  // 0 = English, 1 = Russian

    // Block schedule settings
    bool blockScheduleEnabled_{false};
    int blockScheduleStartHour_{9};
    int blockScheduleStartMin_{0};
    int blockScheduleEndHour_{18};
    int blockScheduleEndMin_{0};

    // Validation state
    bool hasValidationError_{false};
    std::string validationMessage_;

    // Callback for Export Log button
    std::function<void()> exportLogCallback_;

    // Load current settings into form fields
    void loadSettings();

    // Save form fields to settings
    void saveSettings();

    // Validate form inputs
    bool validateInputs();

    // Render sections
    void renderLanguageSettings();
    void renderBlockScheduleSettings();
    void renderYouTrackSettings();
    void renderActivityAliases();
    void renderKTalkSettings();
};

} // namespace timetracker::ui::widgets
