#include "AutoFillService.hpp"

#include "localization/LocalizationManager.hpp"
#include "utils/TimeUtils.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <stdexcept>

namespace timetracker::services {

namespace {

constexpr int64_t kHalfHourSeconds = 30 * 60;

// A range wider than this is almost certainly a mistake and would take a long time to write.
constexpr int kMaxDaysPerRun = 366;

// Three-way overlaps can create a new overlap after being cut apart; a handful of passes is
// plenty to reach a stable layout.
constexpr int kMaxBalancePasses = 5;

} // namespace

AutoFillService::AutoFillService(TimeTrackingService& timeService) : timeService_(timeService) {}

std::string AutoFillService::validate(const AutoFillOptions& options) {
    auto& L = localization::L10n();

    if (options.rangeStart > options.rangeEnd) {
        return L.get("Start date must not be after end date");
    }
    if (options.allocations.empty()) {
        return L.get("Add at least one activity");
    }

    int percentSum = 0;
    for (const auto& allocation : options.allocations) {
        if (allocation.activityName.empty()) {
            return L.get("Activity name cannot be empty");
        }
        if (allocation.percent < 0 || allocation.percent > 100) {
            return L.get("Each percentage must be between 0 and 100");
        }
        percentSum += allocation.percent;
    }
    if (percentSum <= 0) {
        return L.get("Percentages must sum to more than zero");
    }

    if (options.availableMinutes <= 0) {
        return L.get("Available hours must be greater than zero");
    }
    if (options.dayStartMinutes < 0 || options.dayStartMinutes > 23 * 60 + 59) {
        return L.get("Earliest start time must be a valid time of day");
    }
    if (options.gridMinutes < 1 || options.gridMinutes > 60) {
        return L.get("Grid step must be between 1 and 60 minutes");
    }
    if (options.minBlockMinutes < 1) {
        return L.get("Minimum block must be at least one minute");
    }

    return "";
}

int64_t AutoFillService::snapToGrid(int64_t timestamp, int64_t gridSeconds) {
    // The grid is anchored to local midnight so boundaries land on readable clock times.
    int64_t midnight = utils::TimeUtils::startOfDay(timestamp);
    int64_t offset = timestamp - midnight;
    int64_t snapped = ((offset + gridSeconds / 2) / gridSeconds) * gridSeconds;
    return midnight + snapped;
}

int64_t AutoFillService::nextDayStart(int64_t dayStart) {
    std::tm tm = utils::TimeUtils::toLocalTime(dayStart);
    tm.tm_mday += 1;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return static_cast<int64_t>(std::mktime(&tm));
}

int64_t AutoFillService::chooseOverlapBoundary(int64_t overlapStart, int64_t overlapEnd) {
    int64_t midpoint = overlapStart + (overlapEnd - overlapStart) / 2;
    midpoint -= midpoint % 60;  // whole minutes only

    int64_t midnight = utils::TimeUtils::startOfDay(overlapStart);
    int64_t startOffset = overlapStart - midnight;
    // First whole hour or half hour at or after the start of the overlap.
    int64_t firstMark = midnight + ((startOffset + kHalfHourSeconds - 1) / kHalfHourSeconds) * kHalfHourSeconds;

    int64_t best = -1;
    for (int64_t mark = firstMark; mark <= overlapEnd; mark += kHalfHourSeconds) {
        if (best < 0 || std::llabs(mark - midpoint) < std::llabs(best - midpoint)) {
            best = mark;
        }
    }

    // No round boundary inside the overlap — split it down the middle instead.
    return best >= 0 ? best : midpoint;
}

void AutoFillService::balanceOverlaps(std::vector<models::Fact>& facts, AutoFillResult& result) {
    std::vector<int64_t> clipped;

    for (int pass = 0; pass < kMaxBalancePasses; ++pass) {
        std::sort(facts.begin(), facts.end(), [](const models::Fact& lhs, const models::Fact& rhs) {
            if (lhs.startTime != rhs.startTime) {
                return lhs.startTime < rhs.startTime;
            }
            return *lhs.endTime < *rhs.endTime;
        });

        bool changed = false;
        for (std::size_t i = 0; i + 1 < facts.size(); ++i) {
            models::Fact& a = facts[i];
            models::Fact& b = facts[i + 1];
            if (*a.endTime <= b.startTime) {
                continue;
            }

            int64_t boundary;
            if (*b.endTime <= *a.endTime) {
                // b sits entirely inside a: a keeps only the part before b and its tail becomes
                // free time again.
                boundary = b.startTime;
            } else {
                boundary = chooseOverlapBoundary(b.startTime, *a.endTime);
            }

            if (*a.endTime != boundary) {
                a.endTime = boundary;
                clipped.push_back(a.id);
                changed = true;
            }
            if (b.startTime != boundary && boundary < *b.endTime) {
                b.startTime = boundary;
                clipped.push_back(b.id);
                changed = true;
            }
        }

        // Anything cut down to nothing is dropped rather than persisted as an empty interval.
        std::vector<models::Fact> survivors;
        survivors.reserve(facts.size());
        for (const auto& fact : facts) {
            if (*fact.endTime > fact.startTime) {
                survivors.push_back(fact);
            } else {
                timeService_.deleteEntry(fact.id);
                result.factsRemoved++;
                changed = true;
            }
        }
        facts.swap(survivors);

        if (!changed) {
            break;
        }
    }

    std::sort(clipped.begin(), clipped.end());
    clipped.erase(std::unique(clipped.begin(), clipped.end()), clipped.end());

    for (const auto& fact : facts) {
        if (std::binary_search(clipped.begin(), clipped.end(), fact.id)) {
            // updateEntry takes the timestamps from the fact it is handed.
            timeService_.updateEntry(fact, fact.activityName, fact.description);
            result.factsClipped++;
        }
    }
}

std::vector<AutoFillService::Interval> AutoFillService::buildGaps(const std::vector<models::Fact>& facts,
                                                                  int64_t windowStart,
                                                                  int64_t windowEnd,
                                                                  int64_t minBlockSeconds) {
    if (windowEnd <= windowStart) {
        return {};
    }

    std::vector<Interval> occupied;
    occupied.reserve(facts.size());
    for (const auto& fact : facts) {
        int64_t start = std::max(fact.startTime, windowStart);
        int64_t end = std::min(*fact.endTime, windowEnd);
        if (end > start) {
            occupied.push_back({start, end});
        }
    }

    std::sort(occupied.begin(), occupied.end(), [](const Interval& lhs, const Interval& rhs) {
        return lhs.start < rhs.start;
    });

    std::vector<Interval> gaps;
    int64_t cursor = windowStart;
    for (const auto& interval : occupied) {
        if (interval.start > cursor) {
            gaps.push_back({cursor, interval.start});
        }
        cursor = std::max(cursor, interval.end);
    }
    if (cursor < windowEnd) {
        gaps.push_back({cursor, windowEnd});
    }

    // Slivers between back-to-back meetings are not worth an entry.
    gaps.erase(std::remove_if(gaps.begin(), gaps.end(),
                              [minBlockSeconds](const Interval& gap) {
                                  return gap.end - gap.start < minBlockSeconds;
                              }),
               gaps.end());
    return gaps;
}

std::vector<int64_t> AutoFillService::computeQuotas(int64_t totalFree,
                                                    const models::AutoFillProfile& allocations,
                                                    int64_t gridSeconds) {
    const std::size_t count = allocations.size();
    std::vector<int64_t> quotas(count, 0);
    if (count == 0 || totalFree <= 0) {
        return quotas;
    }

    int percentSum = 0;
    for (const auto& allocation : allocations) {
        percentSum += allocation.percent;
    }
    if (percentSum <= 0) {
        return quotas;
    }

    // Hand out whole grid cells by largest remainder so the parts add back up to totalFree.
    const int64_t gridCells = totalFree / gridSeconds;
    const int64_t subGridRemainder = totalFree % gridSeconds;

    std::vector<int64_t> cells(count, 0);
    std::vector<double> fractions(count, 0.0);
    int64_t handedOut = 0;
    for (std::size_t i = 0; i < count; ++i) {
        double exact = static_cast<double>(gridCells) * allocations[i].percent / percentSum;
        cells[i] = static_cast<int64_t>(std::floor(exact));
        fractions[i] = exact - static_cast<double>(cells[i]);
        handedOut += cells[i];
    }

    std::vector<std::size_t> order(count);
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(), [&](std::size_t lhs, std::size_t rhs) {
        if (fractions[lhs] != fractions[rhs]) {
            return fractions[lhs] > fractions[rhs];
        }
        if (allocations[lhs].percent != allocations[rhs].percent) {
            return allocations[lhs].percent > allocations[rhs].percent;
        }
        return lhs < rhs;  // profile order breaks the last tie, so runs are reproducible
    });

    for (int64_t i = 0; i < gridCells - handedOut && i < static_cast<int64_t>(count); ++i) {
        cells[order[static_cast<std::size_t>(i)]]++;
    }

    for (std::size_t i = 0; i < count; ++i) {
        quotas[i] = cells[i] * gridSeconds;
    }

    // Whatever does not divide into the grid goes to the largest share.
    if (subGridRemainder > 0) {
        std::size_t largest = 0;
        for (std::size_t i = 1; i < count; ++i) {
            if (allocations[i].percent > allocations[largest].percent) {
                largest = i;
            }
        }
        quotas[largest] += subGridRemainder;
    }

    return quotas;
}

std::vector<AutoFillService::PlannedBlock> AutoFillService::layOutBlocks(const std::vector<Interval>& gaps,
                                                                        std::vector<int64_t> quotas,
                                                                        const models::AutoFillProfile& allocations,
                                                                        int64_t gridSeconds,
                                                                        int64_t minBlockSeconds) {
    std::vector<PlannedBlock> blocks;
    if (allocations.empty()) {
        return blocks;
    }

    auto pickActivity = [&]() -> std::size_t {
        std::size_t best = 0;
        for (std::size_t i = 1; i < quotas.size(); ++i) {
            if (quotas[i] > quotas[best]) {
                best = i;
            }
        }
        if (quotas[best] > 0) {
            return best;
        }
        // Snapping can consume a little more than a quota, leaving every quota at zero with gap
        // time still to place. Give the leftover to the largest share.
        std::size_t largest = 0;
        for (std::size_t i = 1; i < allocations.size(); ++i) {
            if (allocations[i].percent > allocations[largest].percent) {
                largest = i;
            }
        }
        return largest;
    };

    int64_t remainingBudget = std::accumulate(quotas.begin(), quotas.end(), int64_t{0});

    for (const auto& gap : gaps) {
        if (remainingBudget <= 0) {
            break;
        }
        // The budget can run out mid-gap; everything past that point stays free.
        const int64_t gapEnd = std::min(gap.end, gap.start + remainingBudget);
        if (gapEnd - gap.start < minBlockSeconds) {
            // Rounding this sliver up to a whole block would overshoot the budget, and the caller
            // cares more about the total than about the last few minutes. Drop it.
            break;
        }

        int64_t cursor = gap.start;
        while (cursor < gapEnd) {
            const std::size_t index = pickActivity();
            const int64_t remaining = gapEnd - cursor;
            int64_t want = std::min(quotas[index], remaining);
            if (want <= 0) {
                want = remaining;
            }

            int64_t cut = cursor + want;
            if (cut < gapEnd) {
                cut = snapToGrid(cut, gridSeconds);
                if (cut - cursor < minBlockSeconds) {
                    cut = cursor + minBlockSeconds;
                }
                if (gapEnd - cut < minBlockSeconds) {
                    cut = gapEnd;  // absorb a tail too small to stand on its own
                }
                cut = std::min(cut, gapEnd);
            }

            blocks.push_back({index, cursor, cut});
            quotas[index] = std::max<int64_t>(0, quotas[index] - (cut - cursor));
            remainingBudget -= cut - cursor;
            cursor = cut;
        }
    }

    // Adjacent blocks of the same activity read better as one entry.
    std::vector<PlannedBlock> merged;
    for (const auto& block : blocks) {
        if (!merged.empty() && merged.back().allocationIndex == block.allocationIndex &&
            merged.back().end == block.start) {
            merged.back().end = block.end;
        } else {
            merged.push_back(block);
        }
    }
    return merged;
}

void AutoFillService::processDay(int64_t dayStart, const AutoFillOptions& options, AutoFillResult& result) {
    const int64_t dayEnd = nextDayStart(dayStart);
    auto facts = timeService_.getEntriesOverlappingRange(dayStart, dayEnd);

    // A running entry has no end, so neither the overlap cut nor the free-time maths apply.
    for (const auto& fact : facts) {
        if (fact.isOngoing()) {
            result.daysSkippedOngoing++;
            return;
        }
    }

    std::vector<models::Fact> kept;
    kept.reserve(facts.size());
    for (const auto& fact : facts) {
        // Previously generated blocks are rebuilt, except ones already sent to YouTrack — those
        // stay put and count as occupied time.
        if (fact.autoGenerated && !fact.exportedToYoutrack) {
            timeService_.deleteEntry(fact.id);
            result.factsRemoved++;
        } else {
            kept.push_back(fact);
        }
    }

    balanceOverlaps(kept, result);
    result.daysProcessed++;

    const int64_t gridSeconds = static_cast<int64_t>(options.gridMinutes) * 60;
    const int64_t minBlockSeconds = static_cast<int64_t>(options.minBlockMinutes) * 60;

    // Everything already logged eats into the budget, wherever it sits — night work before the
    // earliest start time counts too. An entry straddling midnight only contributes its part
    // inside this day, so the two days it touches still add up correctly.
    int64_t occupied = 0;
    for (const auto& fact : kept) {
        const int64_t start = std::max(fact.startTime, dayStart);
        const int64_t end = std::min(*fact.endTime, dayEnd);
        if (end > start) {
            occupied += end - start;
        }
    }

    const int64_t budget = static_cast<int64_t>(options.availableMinutes) * 60;
    const int64_t desired = budget - occupied;
    if (desired <= 0) {
        result.daysAlreadyFull++;
        return;
    }
    if (desired < minBlockSeconds) {
        return;  // the gap to the budget is too small to be worth an entry
    }

    // New blocks never start before the configured time, however early the day's first entry is.
    // The upper bound is midnight: only this day's entries were loaded, so writing past it could
    // silently overlap tomorrow.
    const int64_t windowStart = dayStart + static_cast<int64_t>(options.dayStartMinutes) * 60;
    const auto gaps = buildGaps(kept, windowStart, dayEnd, minBlockSeconds);

    int64_t totalFree = 0;
    for (const auto& gap : gaps) {
        totalFree += gap.end - gap.start;
    }

    const auto quotas = computeQuotas(std::min(desired, totalFree), options.allocations, gridSeconds);
    const auto blocks = layOutBlocks(gaps, quotas, options.allocations, gridSeconds, minBlockSeconds);

    int64_t placed = 0;
    for (const auto& block : blocks) {
        timeService_.addManualEntry(options.allocations[block.allocationIndex].activityName,
                                    block.start, block.end, "", /*autoGenerated=*/true);
        result.blocksCreated++;
        placed += block.end - block.start;
    }

    // Either the day ran out of free time or the last sliver was too short to place.
    if (placed < desired) {
        result.daysShortOfBudget++;
    }
}

AutoFillResult AutoFillService::run(const AutoFillOptions& options) {
    AutoFillResult result;

    std::string validationError = validate(options);
    if (!validationError.empty()) {
        result.errorMessage = validationError;
        return result;
    }

    try {
        int64_t dayStart = utils::TimeUtils::startOfDay(options.rangeStart);
        const int64_t lastDayStart = utils::TimeUtils::startOfDay(options.rangeEnd);

        // An empty list means the caller did not filter; otherwise only the listed days are filled.
        const bool filterDays = !options.enabledDays.empty();

        int guard = 0;
        while (dayStart <= lastDayStart && guard < kMaxDaysPerRun) {
            const bool enabled = !filterDays ||
                std::find(options.enabledDays.begin(), options.enabledDays.end(), dayStart) !=
                    options.enabledDays.end();
            if (enabled) {
                processDay(dayStart, options, result);
            }
            dayStart = nextDayStart(dayStart);
            ++guard;
        }

        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.errorMessage = e.what();
    }

    return result;
}

} // namespace timetracker::services
