#pragma once

#include "services/TimeTrackingService.hpp"
#include <memory>
#include <functional>

namespace timetracker::ui::widgets {

class QuickAddDialog {
public:
    explicit QuickAddDialog(std::shared_ptr<services::TimeTrackingService> timeService);
    ~QuickAddDialog() = default;

    // Render the dialog (call this every frame if visible)
    void render();

    // Show/hide the dialog
    void show();
    void hide();
    bool isVisible() const { return visible_; }

    // Set callback for when tracking starts
    void setOnTrackingStarted(std::function<void()> callback);

    // Set callback for when dialog is closed (OK, Cancel, or Escape)
    void setOnDialogClosed(std::function<void()> callback);

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;
    bool visible_{false};
    char activityInput_[256]{};
    std::function<void()> onTrackingStarted_;
    std::function<void()> onDialogClosed_;

    void renderAutocomplete();
};

} // namespace timetracker::ui::widgets
