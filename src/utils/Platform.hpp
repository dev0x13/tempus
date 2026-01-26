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
        return std::filesystem::current_path() / "data.db";
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

    static bool openFileInEditor(const std::filesystem::path& filePath, std::string& errorMessage) {
#ifdef _WIN32
        // Windows: Use ShellExecute with 'open' action
        HINSTANCE result = ShellExecuteA(
            nullptr,                    // hwnd
            "open",                     // operation
            filePath.string().c_str(),  // file
            nullptr,                    // parameters
            nullptr,                    // directory
            SW_SHOWNORMAL              // show command
        );

        // ShellExecute returns a value > 32 on success
        if (reinterpret_cast<intptr_t>(result) <= 32) {
            errorMessage = "Failed to open file in editor. Error code: " + std::to_string(reinterpret_cast<intptr_t>(result));
            return false;
        }
        return true;
#elif defined(__APPLE__)
        // macOS: Not implemented
        errorMessage = "Opening files in editor is not supported on macOS in this version.";
        return false;
#else
        // Linux: Use xdg-open
        std::string command = "xdg-open \"" + filePath.string() + "\" 2>&1";
        FILE* pipe = popen(command.c_str(), "r");
        if (!pipe) {
            errorMessage = "Failed to execute xdg-open command.";
            return false;
        }

        // Read any error output
        char buffer[256];
        std::string output;
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            output += buffer;
        }

        int exitCode = pclose(pipe);
        if (exitCode != 0) {
            errorMessage = "xdg-open failed with exit code " + std::to_string(exitCode);
            if (!output.empty()) {
                errorMessage += ": " + output;
            }
            return false;
        }
        return true;
#endif
    }
};

} // namespace timetracker::utils
