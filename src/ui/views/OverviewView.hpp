#pragma once

#include "services/TimeTrackingService.hpp"
#include <memory>
#include <string>

namespace timetracker::ui {

class OverviewView {
public:
    explicit OverviewView(std::shared_ptr<services::TimeTrackingService> timeService);
    ~OverviewView() = default;

    void render();

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;

    // Quick start input
    char activityInput_[256]{};

    // Recent entries cache
    std::vector<models::Fact> recentEntries_;
    int64_t lastRefresh_{0};

    void renderCurrentTracking();
    void renderQuickStart();
    void renderRecentEntries();
    void refreshRecentEntries();
};

} // namespace timetracker::ui
