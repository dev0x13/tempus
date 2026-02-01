#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <portable-file-dialogs.h>

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
            return std::filesystem::path(path) / "tempus";
        }
        return std::filesystem::path(".");
#else
        const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
        if (xdgDataHome && xdgDataHome[0] != '\0') {
            return std::filesystem::path(xdgDataHome) / "tempus";
        }

        const char* home = std::getenv("HOME");
        if (!home || home[0] == '\0') {
            struct passwd* pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : ".";
        }
        return std::filesystem::path(home) / ".local" / "share" / "tempus";
#endif
    }

    static std::filesystem::path getDatabasePath() {
        return getDataDirectory() / "data.db";
    }

    static void ensureDataDirectoryExists() {
        // Database is now in executable folder, no separate data directory needed
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

    static std::optional<std::filesystem::path> showSaveFileDialog(
            const std::string& title,
            const std::string& defaultFilename,
            const std::vector<std::string>& filters = {"CSV Files", "*.csv"}) {
        auto result = pfd::save_file(
            title,
            defaultFilename,
            filters,
            pfd::opt::force_overwrite
        ).result();

        if (result.empty()) {
            return std::nullopt;
        }

        std::filesystem::path path(result);

        // Ensure .csv extension if not present
        if (path.extension() != ".csv") {
            path += ".csv";
        }

        return path;
    }
};

} // namespace timetracker::utils
