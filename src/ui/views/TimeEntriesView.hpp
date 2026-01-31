#pragma once

#include "services/TimeTrackingService.hpp"
#include "services/StatisticsService.hpp"
#include "services/ExportService.hpp"
#include "services/YouTrackExportService.hpp"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <atomic>
#include <map>
#include <functional>

namespace timetracker::ui {

class TimeEntriesView {
public:
    TimeEntriesView(
        std::shared_ptr<services::TimeTrackingService> timeService,
        std::shared_ptr<services::StatisticsService> statsService,
        std::shared_ptr<services::ExportService> exportService,
        std::shared_ptr<services::YouTrackExportService> youTrackExportService);
    ~TimeEntriesView() = default;

    void render();

    // Set callbacks for external buttons
    void setExportLogCallback(std::function<void()> callback) { exportLogCallback_ = std::move(callback); }
    void setSettingsCallback(std::function<void()> callback) { settingsCallback_ = std::move(callback); }

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;
    std::shared_ptr<services::StatisticsService> statsService_;
    std::shared_ptr<services::ExportService> exportService_;
    std::shared_ptr<services::YouTrackExportService> youTrackExportService_;

    // Entry list
    std::vector<models::Fact> entries_;
    int64_t displayStartTime_{0};
    int64_t displayEndTime_{0};
    int displayStartDate_[3]{};  // year, month, day
    int displayEndDate_[3]{};

    // Edit form state
    bool showEditForm_{false};
    std::optional<models::Fact> editingFact_;
    char editActivityName_[256]{};
    char editActivityDescription_[512]{};
    int editStartDate_[3]{};  // year, month, day
    int editStartTime_[2]{};  // hour, minute
    int editEndDate_[3]{};
    int editEndTime_[2]{};
    bool editIsOngoing_{false};

    // Add form state
    bool showAddForm_{false};
    char addActivityName_[256]{};
    char addActivityDescription_[512]{};
    int addStartDate_[3]{};
    int addStartTime_[2]{};
    int addEndDate_[3]{};
    int addEndTime_[2]{};
    bool addIsOngoing_{true};

    // Activity breakdown state
    bool showActivityBreakdown_{false};

    // External button callbacks
    std::function<void()> exportLogCallback_;
    std::function<void()> settingsCallback_;

    // YouTrack export state
    bool showExportConfirmation_{false};
    bool showExportProgress_{false};
    bool showExportSuccess_{false};
    bool showExportError_{false};
    std::string exportErrorMessage_;
    std::vector<services::AggregatedWorkItem> pendingWorkItems_;
    int exportedMinutes_{0};
    int exportedItems_{0};

    // Progress tracking
    int currentProgress_{0};
    int totalProgress_{0};
    std::atomic<bool> cancelExport_{false};

    void renderTopButtons();
    void renderDateSelector();
    void renderEntriesScrollableArea();
    void renderDateGroupedEntries();
    void renderFixedFooter();
    void renderEditForm();
    void renderAddForm();
    void renderExportConfirmationDialog();
    void renderExportProgressDialog();
    void renderExportSuccessDialog();
    void renderExportErrorDialog();

    void refreshEntries();
    void startEdit(const models::Fact& fact);
    void saveEdit();
    void deleteEntry();
    void startAdd();
    void saveAdd();
    void performCsvExport();
    void performYouTrackExport();

    void setDateFromTimestamp(int64_t timestamp, int* date, int* time);
    int64_t getTimestampFromDate(const int* date, const int* time);

    // Helper to group entries by date
    std::map<std::string, std::vector<models::Fact>> groupEntriesByDate();
};

} // namespace timetracker::ui
