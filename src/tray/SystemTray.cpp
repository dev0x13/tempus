#include "SystemTray.hpp"
#include "localization/LocalizationManager.hpp"
#include "utils/Platform.hpp"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstdio>

#include "ui/resources/Resources.hpp"

#ifdef _WIN32
#include <shellapi.h>
#include <windowsx.h>
#elif __linux__
#include "ui/resources/TrayIcon.hpp"
#include <gtk/gtk.h>
#include <libayatana-appindicator/app-indicator.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdio>
#include <fstream>
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
  auto current = timeService_->getCurrentTracking();
  std::string currentName = current.has_value() ? current->activityName : "";
  std::vector<std::string> result;
  const int batchSize = 20;
  int offset = 0;
  while (result.size() < 5) {
    auto entries = timeService_->getRecentEntries(batchSize, offset);
    if (entries.empty()) break;
    for (const auto& fact : entries) {
      const std::string& name = fact.activityName;
      if (name == currentName) continue;
      bool found = false;
      for (const auto& r : result) {
        if (r == name) { found = true; break; }
      }
      if (!found) {
        result.push_back(name);
        if (result.size() >= 5) break;
      }
    }
    if (static_cast<int>(entries.size()) < batchSize) break;
    offset += batchSize;
  }
  return result;
}

std::string SystemTray::formatTodayTotal() {
  auto& L = localization::L10n();
  time_t now = std::time(nullptr);
  struct tm* t = std::localtime(&now);
  t->tm_hour = 0;
  t->tm_min = 0;
  t->tm_sec = 0;
  int64_t todayStart = static_cast<int64_t>(std::mktime(t));
  int64_t nowSecs = static_cast<int64_t>(now);

  auto entries = timeService_->getEntriesForRange(todayStart, nowSecs);
  int64_t totalSeconds = 0;
  for (const auto& fact : entries) {
    totalSeconds += fact.getDuration(nowSecs);
  }

  int hours = static_cast<int>(totalSeconds / 3600);
  int minutes = static_cast<int>((totalSeconds % 3600) / 60);
  char buf[32];
  std::snprintf(buf, sizeof(buf), L.get("%dh %02dm"), hours, minutes);
  return L.get("Today: ") + std::string(buf);
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

  auto recentActivities = getRecentActivities();

  HMENU menu = CreatePopupMenu();

  // Show/Hide Window
  AppendMenuW(menu, MF_STRING, 1, utils::Platform::utf8ToWide(L.get("Show window")).c_str());

  // Today's total (non-clickable informational item)
  AppendMenuW(menu, MF_STRING | MF_GRAYED, 0,
              utils::Platform::utf8ToWide(formatTodayTotal()).c_str());

  // Stop Tracking (if tracking)
  if (isTracking_) {
    auto current = timeService_->getCurrentTracking();
    if (current.has_value()) {
      std::string stopStr = L.get("Stop: ") + current->activityName;
      AppendMenuW(menu, MF_STRING, 2, utils::Platform::utf8ToWide(stopStr).c_str());
    }
  }

  // Continue: recent activities (inline, below Stop:)
  if (!recentActivities.empty()) {
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    for (size_t i = 0; i < recentActivities.size(); i++) {
      std::string continueStr = L.get("Continue: ") + recentActivities[i];
      AppendMenuW(menu, MF_STRING, 100 + static_cast<UINT>(i),
                  utils::Platform::utf8ToWide(continueStr).c_str());
    }
  }

  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);

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

  // Write the embedded PNG icon to a temporary directory
  // AppIndicator requires an icon file on disk; we use a temp path so the
  // binary has no external file dependency.
  std::string iconDir = "/tmp/tempus-tray-" + std::to_string(getpid());
  std::string iconPath = iconDir + "/tray_icon.png";
  std::string iconName = "tray_icon";

  if (mkdir(iconDir.c_str(), 0700) == 0) {
    std::ofstream out(iconPath, std::ios::binary);
    if (out.write(reinterpret_cast<const char*>(kTrayIconPng), kTrayIconPngLen)) {
      tempIconDir_ = iconDir;
    } else {
      std::cerr << "Warning: Failed to write tray icon to " << iconPath << std::endl;
      iconName = "application-x-executable";
    }
  } else {
    std::cerr << "Warning: Failed to create temp icon dir " << iconDir << std::endl;
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

  // Tell AppIndicator where to find the icon
  if (!tempIconDir_.empty()) {
    app_indicator_set_icon_theme_path(indicator_, tempIconDir_.c_str());
  }

  // Set status to active (show the indicator)
  app_indicator_set_status(indicator_, APP_INDICATOR_STATUS_ACTIVE);

  // Set initial title/label
  app_indicator_set_title(indicator_, "Tempus");

  // Create and set menu
  createMenuLinux();
  app_indicator_set_menu(indicator_, GTK_MENU(menu_));

  running_ = true;

  return true;
}

void SystemTray::createMenuLinux() {
  auto& L = localization::L10n();

  // Create menu
  menu_ = gtk_menu_new();

  // "Quick Add Activity" menu item
  menuItemQuickAdd_ = gtk_menu_item_new_with_label(L.get("Quick start"));
  g_signal_connect(menuItemQuickAdd_, "activate", G_CALLBACK(onMenuQuickAddActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemQuickAdd_);

  // "Show Window" menu item
  menuItemShow_ = gtk_menu_item_new_with_label(L.get("Show window"));
  g_signal_connect(menuItemShow_, "activate", G_CALLBACK(onMenuShowActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemShow_);

  // Today's total (non-clickable informational item)
  menuItemTodayTotal_ = gtk_menu_item_new_with_label(formatTodayTotal().c_str());
  gtk_widget_set_sensitive(menuItemTodayTotal_, FALSE);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemTodayTotal_);

  // Separator
  GtkWidget *separator1 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), separator1);

  // "Stop Tracking" menu item (initially hidden)
  menuItemStop_ = gtk_menu_item_new_with_label(L.get("Stop"));
  g_signal_connect(menuItemStop_, "activate", G_CALLBACK(onMenuStopActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemStop_);
  gtk_widget_set_visible(menuItemStop_, FALSE); // Hidden until tracking starts

  // Separator before "Continue:" items (hidden until there are recent activities)
  menuItemContinueSeparator_ = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemContinueSeparator_);

  // "Continue: X" menu items — 5 pre-allocated slots, shown/hidden dynamically
  for (int i = 0; i < 5; i++) {
    menuItemContinue_[i] = gtk_menu_item_new_with_label("");
    g_object_set_data(G_OBJECT(menuItemContinue_[i]), "index", GINT_TO_POINTER(i));
    g_signal_connect(menuItemContinue_[i], "activate",
                     G_CALLBACK(onMenuContinueActivate), this);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemContinue_[i]);
  }

  // Separator before Exit
  GtkWidget *separator2 = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), separator2);

  // "Exit" menu item
  menuItemExit_ = gtk_menu_item_new_with_label(L.get("Exit"));
  g_signal_connect(menuItemExit_, "activate", G_CALLBACK(onMenuExitActivate),
                   this);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu_), menuItemExit_);

  // Show all menu items
  gtk_widget_show_all(menu_);

  // Hide dynamic items (gtk_widget_show_all showed them)
  gtk_widget_set_visible(menuItemStop_, FALSE);
  gtk_widget_set_visible(menuItemContinueSeparator_, FALSE);
  for (int i = 0; i < 5; i++) {
    gtk_widget_set_visible(menuItemContinue_[i], FALSE);
  }
}

void SystemTray::updateLinux() {
  if (!running_ || !indicator_)
    return;

  auto& L = localization::L10n();

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
      std::string title = L.get("Tracking: ") + current->activityName;
      app_indicator_set_title(indicator_, title.c_str());

      // Update Stop menu item label with activity name
      std::string stopLabel = L.get("Stop: ") + current->activityName;
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

  // Update "Continue: X" slots
  auto recent = getRecentActivities();
  for (int i = 0; i < 5; i++) {
    if (i < static_cast<int>(recent.size())) {
      std::string label = L.get("Continue: ") + recent[i];
      gtk_menu_item_set_label(GTK_MENU_ITEM(menuItemContinue_[i]), label.c_str());
      g_object_set_data_full(G_OBJECT(menuItemContinue_[i]), "activity",
                             g_strdup(recent[i].c_str()), g_free);
      gtk_widget_set_visible(menuItemContinue_[i], TRUE);
    } else {
      gtk_widget_set_visible(menuItemContinue_[i], FALSE);
    }
  }
  gtk_widget_set_visible(menuItemContinueSeparator_, recent.empty() ? FALSE : TRUE);

  gtk_menu_item_set_label(GTK_MENU_ITEM(menuItemTodayTotal_), formatTodayTotal().c_str());
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
  for (int i = 0; i < 5; i++) {
    if (menuItemContinue_[i]) {
      g_signal_handlers_disconnect_by_data(menuItemContinue_[i], this);
    }
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

  // Remove temp icon files written at init
  if (!tempIconDir_.empty()) {
    std::string iconPath = tempIconDir_ + "/tray_icon.png";
    std::remove(iconPath.c_str());
    rmdir(tempIconDir_.c_str());
    tempIconDir_.clear();
  }

  menuItemQuickAdd_ = nullptr;
  menuItemShow_ = nullptr;
  menuItemTodayTotal_ = nullptr;
  menuItemStop_ = nullptr;
  menuItemContinueSeparator_ = nullptr;
  for (int i = 0; i < 5; i++) {
    menuItemContinue_[i] = nullptr;
  }
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

void SystemTray::onMenuContinueActivate(GtkMenuItem *item, void *user_data) {
  auto *tray = static_cast<SystemTray *>(user_data);
  if (!tray || !tray->timeService_) return;
  const char *name = static_cast<const char *>(
      g_object_get_data(G_OBJECT(item), "activity"));
  if (name && *name) {
    tray->timeService_->startTracking(name);
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
