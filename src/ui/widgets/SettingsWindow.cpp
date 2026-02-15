#include "SettingsWindow.hpp"
#include "localization/LocalizationManager.hpp"
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

    auto& L = localization::L10n();

    // Center the window
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 850), ImGuiCond_Appearing);

    if (ImGui::Begin(L.get("Settings"), &visible_, ImGuiWindowFlags_NoCollapse)) {
        renderLanguageSettings();

        ImGui::Separator();
        ImGui::Spacing();

        renderBlockScheduleSettings();

        ImGui::Separator();
        ImGui::Spacing();

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

        if (ImGui::Button(L.get("Save"), ImVec2(100, 0))) {
            saveSettings();
            hide();
        }

        if (!canSave) {
            ImGui::EndDisabled();
        }

        ImGui::SameLine();
        if (ImGui::Button(L.get("Cancel"), ImVec2(100, 0))) {
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

    // Load language
    std::string lang = settingsService_->getLanguage();
    selectedLanguage_ = (lang == "en") ? 0 : 1;

    // Load block schedule settings
    blockScheduleEnabled_ = settingsService_->getBlockScheduleEnabled();
    int period = settingsService_->getBlockSchedulePeriod();
    if (period == 15) blockSchedulePeriodIndex_ = 0;
    else if (period == 30) blockSchedulePeriodIndex_ = 1;
    else if (period == 60) blockSchedulePeriodIndex_ = 2;
    else if (period == 120) blockSchedulePeriodIndex_ = 3;
    else blockSchedulePeriodIndex_ = 2;

    std::string startTime = settingsService_->getBlockScheduleWorkdayStart();
    if (sscanf(startTime.c_str(), "%d:%d", &blockScheduleStartHour_, &blockScheduleStartMin_) != 2) {
        blockScheduleStartHour_ = 9;
        blockScheduleStartMin_ = 0;
    }
    std::string endTime = settingsService_->getBlockScheduleWorkdayEnd();
    if (sscanf(endTime.c_str(), "%d:%d", &blockScheduleEndHour_, &blockScheduleEndMin_) != 2) {
        blockScheduleEndHour_ = 18;
        blockScheduleEndMin_ = 0;
    }
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

    // Save and apply language
    std::string lang = (selectedLanguage_ == 0) ? "en" : "ru";
    settingsService_->setLanguage(lang);
    localization::Language language = localization::stringToLanguage(lang);
    localization::LocalizationManager::instance().setLanguage(language);

    // Save block schedule settings
    settingsService_->setBlockScheduleEnabled(blockScheduleEnabled_);
    const int periodValues[] = {15, 30, 60, 120};
    settingsService_->setBlockSchedulePeriod(periodValues[blockSchedulePeriodIndex_]);
    char timeBuf[8];
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", blockScheduleStartHour_, blockScheduleStartMin_);
    settingsService_->setBlockScheduleWorkdayStart(timeBuf);
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", blockScheduleEndHour_, blockScheduleEndMin_);
    settingsService_->setBlockScheduleWorkdayEnd(timeBuf);
}

bool SettingsWindow::validateInputs() {
    auto& L = localization::L10n();
    hasValidationError_ = false;
    validationMessage_.clear();

    // Validate URL format (basic check)
    std::string url(youtrackUrl_);
    if (!url.empty() && url.find("http://") != 0 && url.find("https://") != 0) {
        hasValidationError_ = true;
        validationMessage_ = L.get("URL must start with http:// or https://");
        return false;
    }

    // Validate aliases - check for non-empty fields
    for (const auto& alias : aliases_) {
        if (alias.markedForDeletion) continue;

        bool hasActivityName = strlen(alias.activityName) > 0;
        bool hasIssueId = strlen(alias.issueId) > 0;

        if (hasActivityName != hasIssueId) {
            hasValidationError_ = true;
            validationMessage_ = L.get("All alias entries must have both activity name and issue ID");
            return false;
        }
    }

    return true;
}

void SettingsWindow::renderLanguageSettings() {
    auto& L = localization::L10n();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("Language:"));
    ImGui::Spacing();

    const char* languages[] = { L.get("English"), L.get("Russian") };
    ImGui::SetNextItemWidth(200);
    if (ImGui::Combo("##language", &selectedLanguage_, languages, 2)) {
        // Language will be saved when user clicks Save
    }
}

void SettingsWindow::renderYouTrackSettings() {
    auto& L = localization::L10n();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("YouTrack Settings"));
    ImGui::Spacing();

    // URL input
    ImGui::Text("%s", L.get("URL:"));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##youtrack_url", youtrackUrl_, sizeof(youtrackUrl_));
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("YouTrack server URL (e.g., https://youtrack.company.com)"));
    }

    ImGui::Spacing();

    // Token input
    ImGui::Text("%s", L.get("Token:"));
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##youtrack_token", youtrackToken_, sizeof(youtrackToken_), ImGuiInputTextFlags_Password);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("YouTrack permanent token for authentication"));
    }

    ImGui::Spacing();

    // Export Log button
    if (ImGui::Button(L.get("Export Log"), ImVec2(130, 0))) {
        if (exportLogCallback_) {
            exportLogCallback_();
        }
    }
}

void SettingsWindow::renderActivityAliases() {
    auto& L = localization::L10n();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("Activity Aliases"));
    ImGui::Spacing();

    ImGui::Text("%s", L.get("Map activity names to YouTrack issue IDs:"));
    ImGui::Spacing();

    // Table for aliases
    if (ImGui::BeginTable("AliasesTable", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
        ImGui::TableSetupColumn(L.get("Activity Name"), ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn(L.get("Issue ID"), ImGuiTableColumnFlags_WidthStretch);
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
    if (ImGui::Button(L.get("+ Add Alias"), ImVec2(180, 0))) {
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

void SettingsWindow::renderBlockScheduleSettings() {
    auto& L = localization::L10n();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("Block Schedule"));
    ImGui::Spacing();

    ImGui::Checkbox(L.get("Enable block schedule view"), &blockScheduleEnabled_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("Display your workday as a grid of time blocks that you can click to fill in"));
    }

    if (blockScheduleEnabled_) {
        ImGui::Spacing();
        ImGui::Text("%s", L.get("Period length:"));
        ImGui::SameLine();
        const char* periodOptions[] = {
            L.get("15 minutes"),
            L.get("30 minutes"),
            L.get("1 hour"),
            L.get("2 hours")
        };
        ImGui::SetNextItemWidth(150);
        ImGui::Combo("##schedulePeriod", &blockSchedulePeriodIndex_, periodOptions, 4);

        ImGui::Spacing();
        ImGui::Text("%s", L.get("Workday start:"));
        ImGui::SameLine();

        char startHourBuf[8], startMinBuf[8];
        snprintf(startHourBuf, sizeof(startHourBuf), "%02d", blockScheduleStartHour_);
        snprintf(startMinBuf, sizeof(startMinBuf), "%02d", blockScheduleStartMin_);

        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##schedStartHour", startHourBuf, sizeof(startHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(startHourBuf);
            blockScheduleStartHour_ = (val < 0) ? 0 : (val > 23) ? 23 : val;
        }
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##schedStartMin", startMinBuf, sizeof(startMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(startMinBuf);
            blockScheduleStartMin_ = (val < 0) ? 0 : (val > 59) ? 59 : val;
        }

        ImGui::Spacing();
        ImGui::Text("%s", L.get("Workday end:"));
        ImGui::SameLine();

        char endHourBuf[8], endMinBuf[8];
        snprintf(endHourBuf, sizeof(endHourBuf), "%02d", blockScheduleEndHour_);
        snprintf(endMinBuf, sizeof(endMinBuf), "%02d", blockScheduleEndMin_);

        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##schedEndHour", endHourBuf, sizeof(endHourBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(endHourBuf);
            blockScheduleEndHour_ = (val < 0) ? 0 : (val > 23) ? 23 : val;
        }
        ImGui::SameLine();
        ImGui::Text(":");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(40);
        if (ImGui::InputText("##schedEndMin", endMinBuf, sizeof(endMinBuf), ImGuiInputTextFlags_CharsDecimal)) {
            int val = atoi(endMinBuf);
            blockScheduleEndMin_ = (val < 0) ? 0 : (val > 59) ? 59 : val;
        }
    }
}

void SettingsWindow::renderKTalkSettings() {
    auto& L = localization::L10n();
    ImGui::TextColored(ImVec4(0.7f, 0.9f, 1.0f, 1.0f), "%s", L.get("KTalk Import"));
    ImGui::Spacing();

    // Include unplanned meetings checkbox
    ImGui::Checkbox(L.get("Include unplanned meetings"), &ktalkIncludeUnplanned_);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", L.get("When enabled, imports all conferences including those without a title.\nWhen disabled, only imports conferences that have a title."));
    }
}

} // namespace timetracker::ui::widgets
