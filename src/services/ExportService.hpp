#pragma once

#include "repositories/IFactRepository.hpp"
#include <memory>
#include <string>
#include <cstdint>

namespace timetracker::services {

class ExportService {
public:
    explicit ExportService(std::shared_ptr<repositories::IFactRepository> factRepo);
    ~ExportService() = default;

    // Export facts to CSV for a date range
    // Returns the CSV content as a string
    std::string exportToCsv(int64_t startTime, int64_t endTime) const;

    // Export facts to CSV file for a date range
    // Returns true if successful
    bool exportToCsvFile(const std::string& filePath, int64_t startTime, int64_t endTime) const;

private:
    std::shared_ptr<repositories::IFactRepository> factRepo_;

    static std::string escapeCSV(const std::string& str);
};

} // namespace timetracker::services
