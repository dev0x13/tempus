#include "SystemTray.hpp"
#include "localization/LocalizationManager.hpp"
#include "utils/Platform.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

#include "ui/resources/Resources.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windowsx.h>
#elif __linux__
#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <limits.h>
#include <unistd.h>
#endif

namespace timetracker::tray {

SystemTray::SystemTray(
    std::shared_ptr<services::TimeTrackingService> timeService)
    : timeService_(std::move(timeService)) {}

SystemTray::~SystemTray() {
#ifdef _WIN32
  cleanupWindows();
#elif __linux__
  cleanupLinux();
#endif
}

bool SystemTray::init() {
#ifdef _WIN32
  return initWindows();
#elif __linux__
  return initLinux();
#endif
}

void SystemTray::update() {
  // Check current tracking status
  auto current = timeService_->getCurrentTracking();
  bool nowTracking = current.has_value();

  bool statusChanged = (nowTracking != isTracking_);
  isTracking_ = nowTracking;

#ifdef _WIN32
  if (statusChanged) {
    updateWindows();
  }
#elif __linux__
  // On Linux, always update to refresh the time label
  updateLinux();
#endif
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

bool SystemTray::isRunning() const { return running_; }

std::vector<std::string> SystemTray::getRecentActivities() {
  // Get last 5 unique activities
  // For now, return empty - will be implemented when we add activity history to
  // TimeTrackingService
  return {};
}

#ifdef _WIN32

LRESULT CALLBACK SystemTray::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                        LPARAM lParam) {
  SystemTray *tray =
      reinterpret_cast<SystemTray *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

  if (uMsg == WM_TRAYICON) {
    if (LOWORD(lParam) == WM_LBUTTONUP) {
      // Left click - show quick add dialog.
      // Note: On Windows, this shows the main window if hidden because ImGui
      // requires a visible window to render. The dialog will be the focus.
      if (tray && tray->showQuickAddCallback_) {
        tray->showQuickAddCallback_();
      }
    } else if (LOWORD(lParam) == WM_RBUTTONUP) {
      // Right click - show context menu with options to show window,
      // stop tracking, access recent activities, and exit the application.
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
  wc.lpszClassName = L"TempusTrayClass";
  RegisterClassExW(&wc);

  // Create hidden message window
  messageWindow_ = CreateWindowExW(
      0, L"TempusTrayClass", L"TempusTray", 0, 0, 0, 0, 0,
      HWND_MESSAGE, nullptr, GetModuleHandle(nullptr), nullptr);

  if (!messageWindow_) {
    std::cerr << "Failed to create tray message window" << std::endl;
    return false;
  }

  SetWindowLongPtr(messageWindow_, GWLP_USERDATA,
                   reinterpret_cast<LONG_PTR>(this));

  idleIcon_ = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));
  activeIcon_ = LoadIcon(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDI_APPICON));

  // Initialize NOTIFYICONDATA
  ZeroMemory(&nid_, sizeof(nid_));
  nid_.cbSize = sizeof(NOTIFYICONDATAW);
  nid_.hWnd = messageWindow_;
  nid_.uID = TRAY_ID;
  nid_.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
  nid_.uCallbackMessage = WM_TRAYICON;
  nid_.hIcon = idleIcon_;
  wcscpy_s(nid_.szTip, L"Tempus - Idle");

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
  if (!running_)
    return;

  auto& L = localization::L10n();
  nid_.hIcon = isTracking_ ? activeIcon_ : idleIcon_;

  if (isTracking_) {
    auto current = timeService_->getCurrentTracking();
    if (current.has_value()) {
      std::string tipStr = L.get("Tempus - Tracking: ") + current->activityName;
      std::wstring tip = utils::Platform::utf8ToWide(tipStr);
      wcscpy_s(nid_.szTip, tip.c_str());
    }
  } else {
    std::wstring idleTip = utils::Platform::utf8ToWide(L.get("Tempus - Idle"));
    wcscpy_s(nid_.szTip, idleTip.c_str());
  }

  Shell_NotifyIconW(NIM_MODIFY, &nid_);
}

void SystemTray::showContextMenuWindows() {
  auto& L = localization::L10n();
  POINT pt;
  GetCursorPos(&pt);

  HMENU menu = CreatePopupMenu();

  // Show/Hide Window
  AppendMenuW(menu, MF_STRING, 1, utils::Platform::utf8ToWide(L.get("Show window")).c_str());

  // Stop Tracking (if tracking)
  if (isTracking_) {
    auto current = timeService_->getCurrentTracking();
    if (current.has_value()) {
      std::string stopStr = L.get("Stop tracking: ") + current->activityName;
      AppendMenuW(menu, MF_STRING, 2, utils::Platform::utf8ToWide(stopStr).c_str());
    }
  }

  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

  // Recent Activities submenu
  auto recentActivities = getRecentActivities();
  if (!recentActivities.empty()) {
    HMENU recentMenu = CreatePopupMenu();
    for (size_t i = 0; i < recentActivities.size() && i < 5; i++) {
      AppendMenuW(recentMenu, MF_STRING, 100 + static_cast<UINT>(i),
                  utils::Platform::utf8ToWide(recentActivities[i]).c_str());
    }
    AppendMenuW(menu, MF_POPUP, reinterpret_cast<UINT_PTR>(recentMenu),
                utils::Platform::utf8ToWide(L.get("Recent activities")).c_str());
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  }

  // Exit
  AppendMenuW(menu, MF_STRING, 3, utils::Platform::utf8ToWide(L.get("Exit")).c_str());

  // Required for popup menus to work correctly
  SetForegroundWindow(messageWindow_);

  UINT cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0,
                            messageWindow_, nullptr);

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

#elif __linux__

// Linux/libayatana-appindicator implementation
//
// Note: The AppIndicator API does not support left-click vs right-click
// differentiation like Windows. Instead, it only supports a menu-based interface.
// "Quick Add Activity" is placed as the first menu item for quick access.
// Note: Due to ImGui architecture, the main window must be visible to render
// the quick add dialog, so clicking "Quick Add Activity" will show the main window.

bool SystemTray::initLinux() {
  // Initialize GTK (safe to call multiple times)
  if (!gtk_init_check(nullptr, nullptr)) {
    std::cerr << "GTK initialization failed. System tray will not be available."
              << std::endl;
    std::cerr << "The application will continue without tray support."
              << std::endl;
    return true; // Allow app to continue without tray
  }

  // Get the absolute path to the icon file
  // AppIndicator on Unity requires absolute paths
  char exePath[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
  if (len == -1) {
    std::cerr << "Failed to get executable path" << std::endl;
    return true;
  }
  exePath[len] = '\0';

  // Get directory containing executable
  std::string exeDir(exePath);
  size_t lastSlash = exeDir.find_last_of('/');
  if (lastSlash != std::string::npos) {
    exeDir = exeDir.substr(0, lastSlash);
  }

  // Construct icon path (relative to executable)
  std::string iconDir = exeDir + "/assets/icons";
  std::string iconPath = iconDir + "/tray_icon.png";
  std::string iconName = "tray_icon";

  // Verify icon file exists
  bool iconExists = (access(iconPath.c_str(), F_OK) == 0);
  if (!iconExists) {
    std::cerr << "Warning: Icon file not found at: " << iconPath << std::endl;
    std::cerr << "Using fallback icon theme name" << std::endl;
    iconName = "application-x-executable";
  }

  // Create AppIndicator
  indicator_ = app_indicator_new("tempus", iconName.c_str(),
                                 APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

  if (!indicator_) {
    std::cerr
        << "Failed to create AppIndicator. System tray will not be available."
        << std::endl;
    return true; // Allow app to continue without tray
  }

  // Set icon theme path for Unity compatibility
  // This tells AppIndicator where to find icon files
  if (iconExists) {
    app_indicator_set_icon_theme_path(indicator_, iconDir.c_str());
  }

  // Set status to active (show the indicator)
  app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);

  // Set initial title/label
  app_indicator_set_title(indicator_, "Tempus");

  // Create and set menu
  createMenuLinux();
  app_indicator_set_menu(indicator_, GTK_MENU(menu_));

  // Store icon name for updates
  currentIconPath_ = iconName;

  running_ = true;

  return true;
}

void SystemTray::createMenuLinux() {
  auto& L = localization::L10n();

  // Create menu
  menu_ = gtk_menu_new();

  // "Quick Add Activity" menu item
  menuItemQuickAdd_ = gtk_menu_item_new_with_label(L.get("Quick add activity"));
  g_signal_connect(menuItemQuickAdd_, "activate", G_CALLBACK(onMenuQuickAddActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemQuickAdd_);

  // "Show Window" menu item
  menuItemShow_ = gtk_menu_item_new_with_label(L.get("Show window"));
  g_signal_connect(menuItemShow_, "activate", G_CALLBACK(onMenuShowActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemShow_);

  // Separator
  GtkWidget *separator1 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), separator1);

  // "Stop Tracking" menu item (initially hidden)
  menuItemStop_ = gtk_menu_item_new_with_label(L.get("Stop tracking"));
  g_signal_connect(menuItemStop_, "activate", G_CALLBACK(onMenuStopActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemStop_);
  gtk_widget_set_visible(menuItemStop_, FALSE); // Hidden until tracking starts

  // Separator (for when Stop is visible)
  GtkWidget *separator2 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), separator2);

  // "Exit" menu item
  menuItemExit_ = gtk_menu_item_new_with_label(L.get("Exit"));
  g_signal_connect(menuItemExit_, "activate", G_CALLBACK(onMenuExitActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemExit_);

  // Show all menu items
  gtk_widget_show_all(menu_);

  // Hide the stop item again (gtk_widget_show_all showed it)
  gtk_widget_set_visible(menuItemStop_, FALSE);
}

void SystemTray::updateLinux() {
  if (!running_ || !indicator_)
    return;

  // Check current tracking status
  auto current = timeService_->getCurrentTracking();
  bool nowTracking = current.has_value();

  // Update tooltip and menu based on tracking status
  if (nowTracking) {
    if (current.has_value()) {
      // Calculate elapsed time
      int64_t currentTime = std::time(nullptr);
      int64_t elapsedSeconds = current->getDuration(currentTime);

      // Format time as HH:MM
      int hours = elapsedSeconds / 3600;
      int minutes = (elapsedSeconds % 3600) / 60;

      std::ostringstream timeStream;
      timeStream << std::setfill('0') << std::setw(2) << hours
                 << ":" << std::setfill('0') << std::setw(2) << minutes;

      // Create label text: "Activity Name HH:MM"
      std::string label = current->activityName + " " + timeStream.str();
      app_indicator_set_label(indicator_, label.c_str(), nullptr);

      // Update title (shown in some environments)
      std::string title = "Tracking: " + current->activityName;
      app_indicator_set_title(indicator_, title.c_str());

      // Update Stop menu item label with activity name
      std::string stopLabel = "Stop Tracking: " + current->activityName;
      gtk_menu_item_set_label(GTK_MENU_ITEM(menuItemStop_), stopLabel.c_str());
    }

    // Show the Stop menu item
    gtk_widget_set_visible(menuItemStop_, TRUE);
  } else {
    // Update for idle state
    app_indicator_set_label(indicator_, "", nullptr);
    app_indicator_set_title(indicator_, "Tempus");

    // Hide the Stop menu item
    gtk_widget_set_visible(menuItemStop_, FALSE);
  }
}

void SystemTray::cleanupLinux() {
  if (!running_)
    return;

  // Cleanup GTK widgets
  if (menuItemQuickAdd_) {
    g_signal_handlers_disconnect_by_data(menuItemQuickAdd_, this);
  }
  if (menuItemShow_) {
    g_signal_handlers_disconnect_by_data(menuItemShow_, this);
  }
  if (menuItemStop_) {
    g_signal_handlers_disconnect_by_data(menuItemStop_, this);
  }
  if (menuItemExit_) {
    g_signal_handlers_disconnect_by_data(menuItemExit_, this);
  }

  if (menu_) {
    gtk_widget_destroy(menu_);
    menu_ = nullptr;
  }

  if (indicator_) {
    g_object_unref(indicator_);
    indicator_ = nullptr;
  }

  menuItemQuickAdd_ = nullptr;
  menuItemShow_ = nullptr;
  menuItemStop_ = nullptr;
  menuItemExit_ = nullptr;
  running_ = false;
}

// Static GTK signal callbacks

void SystemTray::onMenuQuickAddActivate(GtkMenuItem * /*item*/, void *user_data) {
  auto *tray = static_cast<SystemTray *>(user_data);
  if (tray && tray->showQuickAddCallback_) {
    tray->showQuickAddCallback_();
  }
}

void SystemTray::onMenuShowActivate(GtkMenuItem * /*item*/, void *user_data) {
  auto *tray = static_cast<SystemTray *>(user_data);
  if (tray && tray->showWindowCallback_) {
    tray->showWindowCallback_();
  }
}

void SystemTray::onMenuStopActivate(GtkMenuItem * /*item*/, void *user_data) {
  auto *tray = static_cast<SystemTray *>(user_data);
  if (tray && tray->timeService_) {
    tray->timeService_->stopTracking();
    tray->update();
  }
}

void SystemTray::onMenuExitActivate(GtkMenuItem * /*item*/, void *user_data) {
  auto *tray = static_cast<SystemTray *>(user_data);
  if (tray && tray->exitCallback_) {
    tray->exitCallback_();
  }
}

#endif

} // namespace timetracker::tray
