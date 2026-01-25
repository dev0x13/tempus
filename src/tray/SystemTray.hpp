#pragma once

#include "services/TimeTrackingService.hpp"
#include <memory>
#include <functional>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else
// Forward declarations for Linux/GTK types
typedef struct _AppIndicator AppIndicator;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkMenuItem GtkMenuItem;
#endif

namespace timetracker::tray {

class SystemTray {
public:
    explicit SystemTray(std::shared_ptr<services::TimeTrackingService> timeService);
    ~SystemTray();

    // Non-copyable, non-movable
    SystemTray(const SystemTray&) = delete;
    SystemTray& operator=(const SystemTray&) = delete;
    SystemTray(SystemTray&&) = delete;
    SystemTray& operator=(SystemTray&&) = delete;

    // Initialize the system tray
    bool init();

    // Update the tray icon and menu based on tracking status
    void update();

    // Set callbacks
    void setShowWindowCallback(std::function<void()> callback);
    void setExitCallback(std::function<void()> callback);
    void setShowQuickAddCallback(std::function<void()> callback);

    // Check if tray is running
    bool isRunning() const;

    // Process tray messages (Windows only)
    void processTrayMessage(unsigned int msg, void* wparam, void* lparam);

private:
    std::shared_ptr<services::TimeTrackingService> timeService_;
    std::function<void()> showWindowCallback_;
    std::function<void()> exitCallback_;
    std::function<void()> showQuickAddCallback_;

    bool running_{false};
    bool isTracking_{false};

#ifdef _WIN32
    HWND messageWindow_{nullptr};
    NOTIFYICONDATAW nid_{};
    HICON idleIcon_{nullptr};
    HICON activeIcon_{nullptr};
    static constexpr UINT WM_TRAYICON = WM_USER + 1;
    static constexpr UINT TRAY_ID = 1;

    bool initWindows();
    void cleanupWindows();
    void updateWindows();
    void showContextMenuWindows();
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#else
    // Linux/libayatana-appindicator implementation
    AppIndicator* indicator_{nullptr};
    GtkWidget* menu_{nullptr};
    GtkWidget* menuItemQuickAdd_{nullptr};
    GtkWidget* menuItemShow_{nullptr};
    GtkWidget* menuItemStop_{nullptr};
    GtkWidget* menuItemExit_{nullptr};
    std::string currentIconPath_;

    bool initLinux();
    void cleanupLinux();
    void updateLinux();
    void createMenuLinux();

    // Static GTK signal callbacks
    static void onMenuQuickAddActivate(GtkMenuItem* item, void* user_data);
    static void onMenuShowActivate(GtkMenuItem* item, void* user_data);
    static void onMenuStopActivate(GtkMenuItem* item, void* user_data);
    static void onMenuExitActivate(GtkMenuItem* item, void* user_data);
#endif

    std::vector<std::string> getRecentActivities();
};

} // namespace timetracker::tray
