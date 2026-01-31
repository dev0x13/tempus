#pragma once

#include <string>
#include <cstdint>

namespace timetracker::models {

struct Activity {
    int64_t id{0};
    std::string name;
    std::string searchName;  // Lowercase for case-insensitive search
    std::string description;
    bool deleted{false};

    Activity() = default;

    Activity(int64_t id, std::string name, std::string searchName, std::string description = "", bool deleted = false)
        : id(id), name(std::move(name)), searchName(std::move(searchName)), description(std::move(description)), deleted(deleted) {}

    explicit Activity(std::string name, std::string description = "")
        : name(std::move(name)), description(std::move(description)) {
        // Generate search name (lowercase)
        searchName.reserve(this->name.size());
        for (char c : this->name) {
            searchName += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
    }

    bool operator==(const Activity& other) const {
        return id == other.id;
    }
};

} // namespace timetracker::models
