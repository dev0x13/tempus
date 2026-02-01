#pragma once

#include <string>
#include <vector>
#include <ghc/filesystem.hpp>
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

namespace fs = ghc::filesystem;

namespace timetracker::utils {

class Platform {
public:
#ifdef _WIN32
    // Convert UTF-8 string to UTF-16 wide string for Windows APIs
    static std::wstring utf8ToWide(const std::string& utf8) {
        if (utf8.empty()) return std::wstring();
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
        if (size <= 0) return std::wstring();
        std::wstring wide(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &wide[0], size);
        return wide;
    }
#endif

    static fs::path getDataDirectory() {
#ifdef _WIN32
        char path[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
            return fs::path(path) / "tempus";
        }
        return fs::path(".");
#else
        const char* xdgDataHome = std::getenv("XDG_DATA_HOME");
        if (xdgDataHome && xdgDataHome[0] != '\0') {
            return fs::path(xdgDataHome) / "tempus";
        }

        const char* home = std::getenv("HOME");
        if (!home || home[0] == '\0') {
            struct passwd* pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : ".";
        }
        return fs::path(home) / ".local" / "share" / "tempus";
#endif
    }

    static fs::path getDatabasePath() {
        return getDataDirectory() / "data.db";
    }

    static void ensureDataDirectoryExists() {
        // Database is now in executable folder, no separate data directory needed
    }

    static fs::path getAssetsDirectory() {
        // Try relative to executable first
        fs::path execPath = fs::current_path();
        auto assetsPath = execPath / "assets";
        if (fs::exists(assetsPath)) {
            return assetsPath;
        }

        // Try build directory structure
        assetsPath = execPath / ".." / "assets";
        if (fs::exists(assetsPath)) {
            return fs::canonical(assetsPath);
        }

        return assetsPath;
    }

    static std::optional<fs::path> showSaveFileDialog(
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

        fs::path filePath(result);

        // Ensure .csv extension if not present
        if (filePath.extension() != ".csv") {
            filePath += ".csv";
        }

        return filePath;
    }
};

} // namespace timetracker::utils
