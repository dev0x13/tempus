#pragma once

#include "services/TimeTrackingService.hpp"
#include "ui/widgets/ActivityAutocomplete.hpp"
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
    std::unique_ptr<widgets::ActivityAutocomplete> activityAutocomplete_;

    // Recent entries cache
    std::vector<models::Fact> recentEntries_;
    int64_t lastRefresh_{0};

    void renderCurrentTracking();
    void renderQuickStart();
    void renderRecentEntries();
    void refreshRecentEntries();
};

} // namespace timetracker::ui
