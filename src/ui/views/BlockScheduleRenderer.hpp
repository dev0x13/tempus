#pragma once

#include "models/Fact.hpp"
#include "services/SettingsService.hpp"
#include "services/TimeTrackingService.hpp"
#include "ui/widgets/ActivityAutocomplete.hpp"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace timetracker::ui {

// A segment within a slot: either a fact or an empty gap
struct ScheduleSegment {
    int64_t startTime{0};
    int64_t endTime{0};
    bool isFact{false};
    const models::Fact* fact{nullptr};  // Non-null if isFact is true
};

// User action from the schedule renderer
struct ScheduleAction {
    enum Type { None, EditFact, CreateFact };
    Type type{None};
    const models::Fact* factToEdit{nullptr};  // For EditFact
    int64_t slotStart{0};   // For CreateFact
    int64_t slotEnd{0};     // For CreateFact
};

class BlockScheduleRenderer {
public:
    BlockScheduleRenderer(
        std::shared_ptr<services::SettingsService> settingsService,
        std::shared_ptr<services::TimeTrackingService> timeService);
    ~BlockScheduleRenderer() = default;

    // Render the schedule for a single day's facts.
    // Returns an action if the user interacted with a slot.
    ScheduleAction renderDay(
        const std::string& dateHeader,
        const std::string& isoDate,
        const std::vector<models::Fact>& dayFacts,
        int64_t dayStart);

    // Render the inline quick-add input. Returns true if a fact was created.
    bool renderQuickAdd();

    // Cancel any active quick-add input
    void cancelQuickAdd();

    // Check if quick-add is active
    bool isQuickAddActive() const { return quickAddActive_; }

private:
    std::shared_ptr<services::SettingsService> settingsService_;
    std::shared_ptr<services::TimeTrackingService> timeService_;
    widgets::ActivityAutocomplete autocomplete_;

    // Quick-add state
    bool quickAddActive_{false};
    int64_t quickAddStart_{0};
    int64_t quickAddEnd_{0};
    char quickAddName_[256]{};
    bool quickAddFocusSet_{false};

    // Compute segments for a single slot given overlapping facts
    std::vector<ScheduleSegment> computeSegments(
        int64_t slotStart, int64_t slotEnd,
        const std::vector<models::Fact>& dayFacts) const;

    // Start quick-add for a given time range
    void startQuickAdd(int64_t start, int64_t end);
};

} // namespace timetracker::ui
