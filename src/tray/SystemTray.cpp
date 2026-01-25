#include "SystemTray.hpp"
#include <iostream>

#ifdef _WIN32
#include <shellapi.h>
#include <windowsx.h>
#endif

namespace timetracker::tray {

SystemTray::SystemTray(std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {}

SystemTray::~SystemTray() {
#ifdef _WIN32
    cleanupWindows();
#else
    cleanupLinux();
#endif
}

bool SystemTray::init() {
#ifdef _WIN32
    return initWindows();
#else
    return initLinux();
#endif
}

void SystemTray::update() {
    // Check current tracking status
    auto current = timeService_->getCurrentTracking();
    bool nowTracking = current.has_value();

    if (nowTracking != isTracking_) {
        isTracking_ = nowTracking;
#ifdef _WIN32
        updateWindows();
#else
        updateLinux();
#endif
    }
}

void SystemTray::setShowWindowCallback(std::function<void()> callback) {
    showWindowCallback_ = std::move(callback);
}

void SystemTray::setExitCallback(std::function<void()> callback) {
    exitCallback_ = std::move(callback);
}

void SystemTray::setShowQuickAddCallback(std::function<void()> callback) {
    showQuickAddCallback_ = std::move(callback);
}

bool SystemTray::isRunning() const {
    return running_;
}

std::vector<std::string> SystemTray::getRecentActivities() {
    // Get last 5 unique activities
    // For now, return empty - will be implemented when we add activity history to TimeTrackingService
    return {};
}

#ifdef _WIN32

LRESULT CALLBACK SystemTray::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    SystemTray* tray = reinterpret_cast<SystemTray*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    if (uMsg == WM_TRAYICON) {
        if (LOWORD(lParam) == WM_LBUTTONUP) {
            // Left click - show quick add dialog
            if (tray && tray->showQuickAddCallback_) {
                tray->showQuickAddCallback_();
            }
        } else if (LOWORD(lParam) == WM_RBUTTONUP) {
            // Right click - show context menu
            if (tray) {
                tray->showContextMenuWindows();
            }
        }
        return 0;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool SystemTray::initWindows() {
    // Register window class for message handling
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = L"TimeTrackerTrayClass";
    RegisterClassExW(&wc);

    // Create hidden message window
    messageWindow_ = CreateWindowExW(
        0, L"TimeTrackerTrayClass", L"TimeTrackerTray",
        0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), nullptr
    );

    if (!messageWindow_) {
        std::cerr << "Failed to create tray message window" << std::endl;
        return false;
    }

    SetWindowLongPtr(messageWindow_, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Create simple icons (will use default icon for now)
    idleIcon_ = LoadIcon(nullptr, IDI_APPLICATION);
    activeIcon_ = LoadIcon(nullptr, IDI_INFORMATION);

    // Initialize NOTIFYICONDATA
    ZeroMemory(&nid_, sizeof(nid_));
    nid_.cbSize = sizeof(NOTIFYICONDATAW);
    nid_.hWnd = messageWindow_;
    nid_.uID = TRAY_ID;
    nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid_.uCallbackMessage = WM_TRAYICON;
    nid_.hIcon = idleIcon_;
    wcscpy_s(nid_.szTip, L"Time Tracker - Idle");

    // Add tray icon
    if (!Shell_NotifyIconW(NIM_ADD, &nid_)) {
        std::cerr << "Failed to add tray icon" << std::endl;
        DestroyWindow(messageWindow_);
        return false;
    }

    running_ = true;
    return true;
}

void SystemTray::cleanupWindows() {
    if (running_) {
        Shell_NotifyIconW(NIM_DELETE, &nid_);
        running_ = false;
    }

    if (messageWindow_) {
        DestroyWindow(messageWindow_);
        messageWindow_ = nullptr;
    }
}

void SystemTray::updateWindows() {
    if (!running_) return;

    nid_.hIcon = isTracking_ ? activeIcon_ : idleIcon_;

    if (isTracking_) {
        auto current = timeService_->getCurrentTracking();
        if (current.has_value()) {
            std::wstring tip = L"Time Tracker - Tracking: ";
            // Convert activity name to wide string
            std::string activityName = current->activityName;
            tip += std::wstring(activityName.begin(), activityName.end());
            wcscpy_s(nid_.szTip, tip.c_str());
        }
    } else {
        wcscpy_s(nid_.szTip, L"Time Tracker - Idle");
    }

    Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void SystemTray::showContextMenuWindows() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU menu = CreatePopupMenu();

    // Show/Hide Window
    AppendMenuW(menu, MF_STRING, 1, L"Show Window");

    // Stop Tracking (if tracking)
    if (isTracking_) {
        auto current = timeService_->getCurrentTracking();
        if (current.has_value()) {
            std::wstring stopText = L"Stop Tracking: ";
            std::string activityName = current->activityName;
            stopText += std::wstring(activityName.begin(), activityName.end());
            AppendMenuW(menu, MF_STRING, 2, stopText.c_str());
        }
    }

    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

    // Recent Activities submenu
    auto recentActivities = getRecentActivities();
    if (!recentActivities.empty()) {
        HMENU recentMenu = CreatePopupMenu();
        for (size_t i = 0; i < recentActivities.size() && i < 5; i++) {
            std::wstring activityW(recentActivities[i].begin(), recentActivities[i].end());
            AppendMenuW(recentMenu, MF_STRING, 100 + static_cast<UINT>(i), activityW.c_str());
        }
        AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(recentMenu), L"Recent Activities");
        AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    }

    // Exit
    AppendMenuW(menu, MF_STRING, 3, L"Exit");

    // Required for popup menus to work correctly
    SetForegroundWindow(messageWindow_);

    UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, messageWindow_, nullptr);

    if (cmd == 1) {
        // Show Window
        if (showWindowCallback_) {
            showWindowCallback_();
        }
    } else if (cmd == 2) {
        // Stop Tracking
        timeService_->stopTracking();
        update();
    } else if (cmd == 3) {
        // Exit
        if (exitCallback_) {
            exitCallback_();
        }
    } else if (cmd >= 100 && cmd < 105) {
        // Start recent activity
        size_t index = cmd - 100;
        if (index < recentActivities.size()) {
            timeService_->startTracking(recentActivities[index]);
            update();
        }
    }

    DestroyMenu(menu);
}

#else

// Linux/GNOME stub implementation
bool SystemTray::initLinux() {
    std::cerr << "System tray not yet implemented for Linux/GNOME" << std::endl;
    // For now, just return true to allow app to continue without tray
    return true;
}

void SystemTray::cleanupLinux() {
    // Stub
}

void SystemTray::updateLinux() {
    // Stub
}

#endif

} // namespace timetracker::tray
