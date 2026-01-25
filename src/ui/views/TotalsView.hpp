#pragma once

#include "services/StatisticsService.hpp"
#include "services/ExportService.hpp"
#include "services/YouTrackExportService.hpp"
#include <memory>
#include <vector>
#include <string>

namespace timetracker::ui {

class TotalsView {
public:
    TotalsView(
        std::shared_ptr<services::StatisticsService> statsService,
        std::shared_ptr<services::ExportService> exportService,
        std::shared_ptr<services::YouTrackExportService> youTrackExportService);
    ~TotalsView() = default;

    void render();

private:
    std::shared_ptr<services::StatisticsService> statsService_;
    std::shared_ptr<services::ExportService> exportService_;
    std::shared_ptr<services::YouTrackExportService> youTrackExportService_;

    // Date range
    int64_t displayStartTime_{0};
    int64_t displayEndTime_{0};
    int displayStartDate_[3]{};  // year, month, day
    int displayEndDate_[3]{};

    // Statistics cache
    models::Statistics stats_;
    bool statsValid_{false};

    // YouTrack export state
    bool showExportConfirmation_{false};
    bool showExportProgress_{false};
    bool showExportSuccess_{false};
    bool showExportError_{false};
    std::string exportErrorMessage_;
    std::vector<services::AggregatedWorkItem> pendingWorkItems_;
    int exportedMinutes_{0};
    int exportedItems_{0};

    void renderDateSelector();
    void renderTabs();
    void renderActivityTotals();
    void renderDailyTotals();
    void renderWeeklyTotals();
    void renderMonthlyTotals();
    void renderExportButton();
    void renderYouTrackExportButton();
    void renderExportConfirmationDialog();
    void renderExportProgressDialog();
    void renderExportSuccessDialog();
    void renderExportErrorDialog();
    void renderResetExportStatusButton();
    void refreshStatistics();
    void performExport();
    void performYouTrackExport();
    void performResetExportStatus();
};

} // namespace timetracker::ui
