#pragma once

#include "services/KTalkImportService.hpp"
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

private:
    std::shared_ptr<services::KTalkImportService> importService_;
    bool visible_{false};

    // Date range state (array of 3 ints: [year, month, day])
    int fromDate_[3];
    int toDate_[3];

    // Fetch payload input
    char fetchPayload_[8192];  // Large buffer for payload

    // Import state
    bool isImporting_{false};
    std::string statusMessage_;
    bool showSuccess_{false};
    bool showError_{false};

    // Initialize default date range (last 30 days)
    void initializeDefaultDates();

    // Convert date array to YYYY-MM-DD HH:MM:SS string
    // isEndDate: false for 00:00:00 (start of day), true for 23:59:59 (end of day)
    std::string dateToString(const int* date, bool isEndDate);

    // Handle import button click
    void handleImport();
};

}  // namespace widgets
}  // namespace ui
}  // namespace timetracker
