#include "ExportService.hpp"
#include "utils/TimeUtils.hpp"
#include <fstream>
#include <sstream>

namespace timetracker::services {

ExportService::ExportService(std::shared_ptr<repositories::IFactRepository> factRepo)
    : factRepo_(std::move(factRepo)) {}

std::string ExportService::escapeCSV(const std::string& str) {
    bool needsQuotes = false;
    for (char c : str) {
        if (c == '"' || c == ',' || c == '\n' || c == '\r') {
            needsQuotes = true;
            break;
        }
    }

    if (!needsQuotes) {
        return str;
    }

    std::string result = "\"";
    for (char c : str) {
        if (c == '"') {
            result += "\"\"";
        } else {
            result += c;
        }
    }
    result += "\"";
    return result;
}

std::string ExportService::exportToCsv(int64_t startTime, int64_t endTime) const {
    auto facts = factRepo_->findByDateRange(startTime, endTime);

    std::ostringstream oss;

    // Header
    oss << "Activity,Start,End,Duration (seconds),Duration\n";

    int64_t now = utils::TimeUtils::now();

    for (const auto& fact : facts) {
        // Activity name
        oss << escapeCSV(fact.activityName) << ",";

        // Start time
        oss << utils::TimeUtils::formatDateTime(fact.startTime) << ",";

        // End time
        if (fact.endTime.has_value()) {
            oss << utils::TimeUtils::formatDateTime(*fact.endTime);
        } else {
            oss << "(ongoing)";
        }
        oss << ",";

        // Duration in seconds
        int64_t duration = fact.getDuration(now);
        oss << duration << ",";

        // Duration formatted
        oss << utils::TimeUtils::formatDuration(duration);

        oss << "\n";
    }

    return oss.str();
}

bool ExportService::exportToCsvFile(const std::string& filePath, int64_t startTime, int64_t endTime) const {
    std::string content = exportToCsv(startTime, endTime);

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    return file.good();
}

} // namespace timetracker::services
