#pragma once

#include "services/StatisticsService.hpp"
#include "services/ExportService.hpp"
#include <memory>

namespace timetracker::ui {

class TotalsView {
public:
    TotalsView(
        std::shared_ptr<services::StatisticsService> statsService,
        std::shared_ptr<services::ExportService> exportService);
    ~TotalsView() = default;

    void render();

private:
    std::shared_ptr<services::StatisticsService> statsService_;
    std::shared_ptr<services::ExportService> exportService_;

    // Date range
    int64_t displayStartTime_{0};
    int64_t displayEndTime_{0};

    // Statistics cache
    models::Statistics stats_;
    bool statsValid_{false};

    // Export state
    char exportPath_[512]{};
    bool showExportDialog_{false};

    void renderDateSelector();
    void renderTabs();
    void renderActivityTotals();
    void renderDailyTotals();
    void renderWeeklyTotals();
    void renderMonthlyTotals();
    void renderExportButton();
    void renderExportDialog();
    void refreshStatistics();
};

} // namespace timetracker::ui
