#pragma once

#include <string>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <cstdlib>
#include <pwd.h>
#include <unistd.h>
#endif

namespace timetracker::utils {

class Platform {
public:
    static std::filesystem::path getDataDirectory() {
#ifdef _WIN32
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
            return std::filesystem::path(path) / "time-tracker";
        }
        return std::filesystem::path(".");
#else
        const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
        if (xdgDataHome && xdgDataHome[0] != '\0') {
            return std::filesystem::path(xdgDataHome) / "time-tracker";
        }

        const char* home = std::getenv("HOME");
        if (!home || home[0] == '\0') {
            struct passwd* pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : ".";
        }
        return std::filesystem::path(home) / ".local" / "share" / "time-tracker";
#endif
    }

    static std::filesystem::path getDatabasePath() {
        return getDataDirectory() / "data.db";
    }

    static void ensureDataDirectoryExists() {
        auto dataDir = getDataDirectory();
        if (!std::filesystem::exists(dataDir)) {
            std::filesystem::create_directories(dataDir);
        }
    }

    static std::filesystem::path getAssetsDirectory() {
        // Try relative to executable first
        std::filesystem::path execPath = std::filesystem::current_path();
        auto assetsPath = execPath / "assets";
        if (std::filesystem::exists(assetsPath)) {
            return assetsPath;
        }

        // Try build directory structure
        assetsPath = execPath / ".." / "assets";
        if (std::filesystem::exists(assetsPath)) {
            return std::filesystem::canonical(assetsPath);
        }

        return assetsPath;
    }
};

} // namespace timetracker::utils
