#pragma once

#include <string>
#include <cstdint>

namespace timetracker::models {

struct Activity {
    int64_t id{0};
    std::string name;
    std::string searchName;  // Lowercase for case-insensitive search

    Activity() = default;

    Activity(int64_t id, std::string name, std::string searchName)
        : id(id), name(std::move(name)), searchName(std::move(searchName)) {}

    explicit Activity(std::string name)
        : name(std::move(name)) {
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
