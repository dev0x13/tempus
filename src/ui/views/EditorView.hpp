#pragma once

#include "services/TimeTrackingService.hpp"
#include <memory>
#include <string>
#include <optional>

namespace timetracker::ui {

class EditorView {
public:
    explicit EditorView(std::shared_ptr<services::TimeTrackingService> timeService);
    ~EditorView() = default;

    void render();

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;

    // Entry list
    std::vector<models::Fact> entries_;
    int64_t displayStartTime_{0};
    int64_t displayEndTime_{0};
    int selectedEntry_{-1};

    // Edit form state
    bool showEditForm_{false};
    std::optional<models::Fact> editingFact_;
    char editActivityName_[256]{};
    int editStartDate_[3]{};  // year, month, day
    int editStartTime_[2]{};  // hour, minute
    int editEndDate_[3]{};
    int editEndTime_[2]{};
    bool editIsOngoing_{false};

    // Add form state
    bool showAddForm_{false};
    char addActivityName_[256]{};
    int addStartDate_[3]{};
    int addStartTime_[2]{};
    int addEndDate_[3]{};
    int addEndTime_[2]{};

    void renderEntryList();
    void renderEditForm();
    void renderAddForm();
    void refreshEntries();
    void startEdit(const models::Fact& fact);
    void saveEdit();
    void deleteEntry();
    void startAdd();
    void saveAdd();
    void setDateFromTimestamp(int64_t timestamp, int* date, int* time);
    int64_t getTimestampFromDate(const int* date, const int* time);
};

} // namespace timetracker::ui
