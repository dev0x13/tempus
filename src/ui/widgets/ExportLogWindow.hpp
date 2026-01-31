#pragma once

#include "repositories/YouTrackExportLogRepository.hpp"
#include <memory>

namespace timetracker::ui::widgets {

class ExportLogWindow {
public:
    explicit ExportLogWindow(std::shared_ptr<repositories::IYouTrackExportLogRepository> repository);
    ~ExportLogWindow() = default;

    // Render the window (call this every frame if visible)
    void render();

    // Show/hide the window
    void show();
    void hide();
    bool isVisible() const { return visible_; }

private:
    std::shared_ptr<repositories::IYouTrackExportLogRepository> repository_;
    bool visible_{false};

    // Pagination state
    int currentPage_{0};
    int pageSize_{20};
    int totalEntries_{0};
    int totalPages_{0};

    // Confirmation dialog state
    bool showClearConfirmation_{false};

    // Update pagination metadata
    void updatePaginationMetadata();

    // Render the log entries table
    void renderLogTable();

    // Render pagination controls
    void renderPaginationControls();

    // Render clear log button and confirmation dialog
    void renderClearLogControls();

    // Format timestamp to readable string
    std::string formatTimestamp(int64_t timestamp);
};

} // namespace timetracker::ui::widgets
