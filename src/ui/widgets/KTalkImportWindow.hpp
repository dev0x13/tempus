#pragma once

#include "services/KTalkImportService.hpp"
#include <functional>
#include <memory>
#include <string>

namespace timetracker {
namespace ui {
namespace widgets {

class KTalkImportWindow {
public:
    explicit KTalkImportWindow(std::shared_ptr<services::KTalkImportService> importService);
    ~KTalkImportWindow() = default;

    // Render the window (call this every frame if visible)
    void render();

    // Show/hide the window
    void show();
    void hide();
    bool isVisible() const { return visible_; }

    // Set callback for the Open settings button, shown when credentials need attention
    void setOpenSettingsCallback(std::function<void()> callback) { openSettingsCallback_ = std::move(callback); }

private:
    std::shared_ptr<services::KTalkImportService> importService_;
    bool visible_{false};

    // Date range state (array of 3 ints: [year, month, day])
    int fromDate_[3];
    int toDate_[3];

    // Import state
    bool isImporting_{false};
    std::string statusMessage_;
    bool showSuccess_{false};
    bool showError_{false};

    // Set after an authentication failure, so the way to Settings is offered
    bool offerSettings_{false};

    // Opens the settings window, where the space address and token are edited
    std::function<void()> openSettingsCallback_;

    // Initialize default date range (today)
    void initializeDefaultDates();

    // Convert date array to YYYY-MM-DD HH:MM:SS string
    // isEndDate: false for 00:00:00 (start of day), true for 23:59:59 (end of day)
    std::string dateToString(const int* date, bool isEndDate);

    // Handle import button click
    void handleImport();

    // Connection status line, plus a way into Settings when something needs fixing
    void renderConnectionStatus();
};

}  // namespace widgets
}  // namespace ui
}  // namespace timetracker
