#include "SettingsWindow.hpp"
#include "imgui.h"
#include <cstring>
#include <algorithm>

namespace timetracker::ui::widgets {

SettingsWindow::SettingsWindow(std::shared_ptr<services::SettingsService> settingsService)
    : settingsService_(std::move(settingsService)) {
    memset(youtrackUrl_, 0, sizeof(youtrackUrl_));
    memset(youtrackToken_, 0, sizeof(youtrackToken_));
}

void SettingsWindow::render() {
    if (!visible_) return;

    // Center the window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_Appearing);

    if (ImGui::Begin("Settings", &visible_, ImGuiWindowFlags_NoCollapse)) {
        renderYouTrackSettings();

        ImGui::Separator();
        ImGui::Spacing();

        renderActivityAliases();

        ImGui::Separator();
        ImGui::Spacing();

        renderKTalkSettings();

        ImGui::Separator();
        ImGui::Spacing();

        // Validation error message
        if (hasValidationError_) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
            ImGui::TextWrapped("%s", validationMessage_.c_str());
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        // Buttons
        bool canSave = validateInputs();
        if (!canSave) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Save", ImVec2(100, 0))) {
            saveSettings();
            hide();
        }

        if (!canSave) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 0))) {
            hide();
        }
    }
    ImGui::End();

    // Allow closing with Escape
    if (visible_ && ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        hide();
    }
}

void SettingsWindow::show() {
    visible_ = true;
    loadSettings();
}

void SettingsWindow::hide() {
    visible_ = false;
    hasValidationError_ = false;
    validationMessage_.clear();
}

void SettingsWindow::loadSettings() {
    // Load YouTrack URL
    std::string url = settingsService_->getYouTrackUrl();
    strncpy(youtrackUrl_, url.c_str(), sizeof(youtrackUrl_) - 1);
    youtrackUrl_[sizeof(youtrackUrl_) - 1] = '\0';

    // Load YouTrack token
    std::string token = settingsService_->getYouTrackToken();
    strncpy(youtrackToken_, token.c_str(), sizeof(youtrackToken_) - 1);
    youtrackToken_[sizeof(youtrackToken_) - 1] = '\0';

    // Load activity aliases
    aliases_.clear();
    auto aliasMap = settingsService_->getActivityAliases();
    for (const auto& [activity, issueId] : aliasMap) {
        ActivityAlias alias{};
        strncpy(alias.activityName, activity.c_str(), sizeof(alias.activityName) - 1);
        strncpy(alias.issueId, issueId.c_str(), sizeof(alias.issueId) - 1);
        alias.activityName[sizeof(alias.activityName) - 1] = '\0';
        alias.issueId[sizeof(alias.issueId) - 1] = '\0';
        aliases_.push_back(alias);
    }

    // Load KTalk settings
    ktalkIncludeUnplanned_ = settingsService_->getKTalkIncludeUnplanned();
}

void SettingsWindow::saveSettings() {
    // Save YouTrack URL and token
    settingsService_->setYouTrackUrl(youtrackUrl_);
    settingsService_->setYouTrackToken(youtrackToken_);

    // Save activity aliases
    std::map<std::string, std::string> aliasMap;
    for (const auto& alias : aliases_) {
        if (!alias.markedForDeletion && strlen(alias.activityName) > 0 && strlen(alias.issueId) > 0) {
            aliasMap[alias.activityName] = alias.issueId;
        }
    }
    settingsService_->setActivityAliases(aliasMap);

    // Save KTalk settings
    settingsService_->setKTalkIncludeUnplanned(ktalkIncludeUnplanned_);
}

bool SettingsWindow::validateInputs() {
    hasValidationError_ = false;
    validationMessage_.clear();

    // Validate URL format (basic check)
    std::string url(youtrackUrl_);
    if (!url.empty() && url.find("http://") != 0 && url.find("https://") != 0) {
        hasValidationError_ = true;
        validationMessage_ = "URL must start with http:// or https://";
        return false;
    }

    // Validate aliases - check for non-empty fields
    for (const auto& alias : aliases_) {
        if (alias.markedForDeletion) continue;

        bool hasActivityName = strlen(alias.activityName) > 0;
        bool hasIssueId = strlen(alias.issueId) > 0;

        if (hasActivityName != hasIssueId) {
            hasValidationError_ = true;
            validationMessage_ = "All alias entries must have both activity name and issue ID";
            return false;
        }
    }

    return true;
}

void SettingsWindow::renderYouTrackSettings() {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "YouTrack Settings");
    ImGui::Spacing();

    // URL input
    ImGui::Text("URL:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##youtrack_url", youtrackUrl_, sizeof(youtrackUrl_));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("YouTrack server URL (e.g., https://youtrack.company.com)");
    }

    ImGui::Spacing();

    // Token input
    ImGui::Text("Token:");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##youtrack_token", youtrackToken_, sizeof(youtrackToken_), ImGuiInputTextFlags_Password);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("YouTrack permanent token for authentication");
    }

    ImGui::Spacing();

    // Export Log button
    if (ImGui::Button("Export Log", ImVec2(100, 0))) {
        if (exportLogCallback_) {
            exportLogCallback_();
        }
    }
}

void SettingsWindow::renderActivityAliases() {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "Activity Aliases");
    ImGui::Spacing();

    ImGui::Text("Map activity names to YouTrack issue IDs:");
    ImGui::Spacing();

    // Table for aliases
    if (ImGui::BeginTable("AliasesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn("Activity Name", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Issue ID", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableHeadersRow();

        // Render existing aliases
        for (size_t i = 0; i < aliases_.size(); ++i) {
            if (aliases_[i].markedForDeletion) continue;

            ImGui::TableNextRow();

            // Activity name
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i * 2));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##activity", aliases_[i].activityName, sizeof(aliases_[i].activityName));
            ImGui::PopID();

            // Issue ID
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i * 2 + 1));
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##issueId", aliases_[i].issueId, sizeof(aliases_[i].issueId));
            ImGui::PopID();

            // Delete button
            ImGui::TableNextColumn();
            ImGui::PushID(static_cast<int>(i + 1000));
            if (ImGui::Button("X", ImVec2(30, 0))) {
                aliases_[i].markedForDeletion = true;
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::Spacing();

    // Add new alias button
    if (ImGui::Button("+ Add Alias", ImVec2(120, 0))) {
        ActivityAlias newAlias{};
        aliases_.push_back(newAlias);
    }

    // Remove deleted aliases from vector
    aliases_.erase(
        std::remove_if(aliases_.begin(), aliases_.end(),
                      [](const ActivityAlias& a) { return a.markedForDeletion; }),
        aliases_.end()
    );
}

void SettingsWindow::renderKTalkSettings() {
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "KTalk Import");
    ImGui::Spacing();

    // Include unplanned meetings checkbox
    ImGui::Checkbox("Include unplanned meetings", &ktalkIncludeUnplanned_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("When enabled, imports all conferences including those without a title.\nWhen disabled, only imports conferences that have a title.");
    }
}

} // namespace timetracker::ui::widgets
