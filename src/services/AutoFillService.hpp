#pragma once

#include "models/AutoFillProfile.hpp"
#include "models/Fact.hpp"
#include "services/TimeTrackingService.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace timetracker::services {

/**
 * Parameters for a single auto-fill run.
 *
 * `availableMinutes` is a budget for the whole day, not the length of a window: entries that
 * already exist count against it wherever they sit, so a day with 5h of imported meetings and a
 * 9h budget gets 4h of new blocks. The percentages in `allocations` split those 4h.
 */
struct AutoFillOptions {
    int64_t rangeStart{0};       // any timestamp inside the first day to process
    int64_t rangeEnd{0};         // any timestamp inside the last day to process (inclusive)
    int dayStartMinutes{540};    // new blocks never start before this (540 == 09:00); entries that
                                 // already exist earlier still count toward the budget
    int availableMinutes{480};   // total time the day should add up to, existing entries included
    int gridMinutes{15};         // generated boundaries snap to this grid
    int minBlockMinutes{15};     // never create a block shorter than this
    models::AutoFillProfile allocations;

    // Local midnights of the days to fill. Days of the range that are absent here are left
    // untouched — not even their overlaps are resolved. Empty means "every day in the range".
    std::vector<int64_t> enabledDays;
};

struct AutoFillResult {
    bool success{false};
    std::string errorMessage;
    int daysProcessed{0};
    int blocksCreated{0};
    int factsClipped{0};          // existing entries whose bounds were moved to resolve an overlap
    int factsRemoved{0};          // previous auto-fill output that was rebuilt
    int daysSkippedOngoing{0};    // days left alone because tracking was running
    int daysAlreadyFull{0};       // days whose existing entries already meet or beat the budget
    int daysShortOfBudget{0};     // days that ran out of free time before the budget was met
};

/**
 * Tops a day up to a requested number of logged hours, splitting the addition by percentages.
 *
 * Per day the service: drops its own previous output, resolves overlaps between the remaining
 * entries by cutting them at a round boundary, subtracts what is already logged from the budget,
 * and lays the difference into the free time from the earliest start onward. The added time
 * matches the difference exactly; the share of an individual activity may drift by a few minutes
 * because block boundaries snap to a grid.
 */
class AutoFillService {
public:
    explicit AutoFillService(TimeTrackingService& timeService);

    AutoFillResult run(const AutoFillOptions& options);

    // Returns an empty string when the options are usable, otherwise a localized message.
    static std::string validate(const AutoFillOptions& options);

    // Pick the cut point for an overlap spanning [overlapStart, overlapEnd]: a whole hour or half
    // hour inside the overlap when one exists (closest to the middle, earliest on a tie), the
    // midpoint rounded down to a whole minute otherwise. Exposed for verification.
    static int64_t chooseOverlapBoundary(int64_t overlapStart, int64_t overlapEnd);

private:
    struct Interval {
        int64_t start{0};
        int64_t end{0};
    };

    struct PlannedBlock {
        std::size_t allocationIndex{0};
        int64_t start{0};
        int64_t end{0};
    };

    TimeTrackingService& timeService_;

    void processDay(int64_t dayStart, const AutoFillOptions& options, AutoFillResult& result);

    // Cuts overlapping entries apart and writes the changes back. `facts` is updated in place and
    // entries that shrink to nothing are deleted and erased from the vector.
    void balanceOverlaps(std::vector<models::Fact>& facts, AutoFillResult& result);

    static std::vector<Interval> buildGaps(const std::vector<models::Fact>& facts,
                                           int64_t windowStart,
                                           int64_t windowEnd,
                                           int64_t minBlockSeconds);

    // Splits toFill across the profile so that the parts sum to toFill exactly.
    static std::vector<int64_t> computeQuotas(int64_t toFill,
                                              const models::AutoFillProfile& allocations,
                                              int64_t gridSeconds);

    // Places blocks chronologically from the first gap onward and stops once the quotas are spent,
    // so the tail of the day is left empty when the budget is smaller than the free time.
    static std::vector<PlannedBlock> layOutBlocks(const std::vector<Interval>& gaps,
                                                  std::vector<int64_t> quotas,
                                                  const models::AutoFillProfile& allocations,
                                                  int64_t gridSeconds,
                                                  int64_t minBlockSeconds);

    static int64_t snapToGrid(int64_t timestamp, int64_t gridSeconds);
    static int64_t nextDayStart(int64_t dayStart);
};

} // namespace timetracker::services
