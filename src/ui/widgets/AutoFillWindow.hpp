#pragma once

#include "services/AutoFillService.hpp"
#include "services/SettingsService.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace timetracker {
namespace ui {
namespace widgets {

// One editable line of the percentage profile, held ImGui-style in fixed buffers.
struct AutoFillRow {
    char activityName[128]{};
    int percent{0};
    bool markedForDeletion{false};
};

// One day of the selected range, with the tick that decides whether it gets filled.
struct AutoFillDay {
    int64_t dayStart{0};  // local midnight
    int weekday{0};       // 0 == Sunday, same convention as std::tm::tm_wday
    int dayOfMonth{1};
    int month{1};
    bool enabled{true};
};

class AutoFillWindow {
public:
    AutoFillWindow(std::shared_ptr<services::AutoFillService> autoFillService,
                   std::shared_ptr<services::SettingsService> settingsService);

    void render();

    // Opens the window prefilled with the given range (the range currently shown in the main view).
    void show(int64_t rangeStart, int64_t rangeEnd);
    void hide();
    bool isVisible() const { return visible_; }

private:
    std::shared_ptr<services::AutoFillService> autoFillService_;
    std::shared_ptr<services::SettingsService> settingsService_;

    bool visible_{false};

    int fromDate_[3]{};
    int toDate_[3]{};

    int dayStartHour_{9};
    int dayStartMin_{0};
    float availableHours_{8.0f};
    int gridMinutes_{15};
    int minBlockMinutes_{15};

    std::vector<AutoFillRow> rows_;
    std::vector<AutoFillDay> days_;

    std::string statusMessage_;
    bool showSuccess_{false};
    bool showError_{false};

    void loadSettings();
    void saveSettings();
    void handleApply();

    void renderWindowSettings();
    void renderDaySelection();
    void renderAllocations();
    void renderAdvancedSettings();

    // Rebuilds days_ from the current from/to dates, keeping the ticks of days that stay in range.
    void rebuildDayList();

    int totalPercent() const;
    int enabledDayCount() const;
};

}  // namespace widgets
}  // namespace ui
}  // namespace timetracker
