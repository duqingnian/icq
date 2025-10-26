#pragma once

#include "namespaces.h"
#include "../gui/core_dispatcher.h"

CORE_NS_BEGIN

class coll_helper;

CORE_NS_END

UI_NS_BEGIN

class AppConfig
{
public:
    enum class GDPR_Report_To_Server_State
    {
        Reported_Successfully = 0,
        Sent = 1,
        Failed = 2,
        Not_Sent = 3,

        Sent_to_Core = 10000
    };

public:
    AppConfig(const core::coll_helper& collection);

    bool IsContextMenuFeaturesUnlocked() const noexcept;
    bool IsServerHistoryEnabled() const noexcept;
    bool isCrashEnable() const noexcept;
    bool IsTestingEnable() const noexcept;
    bool IsFullLogEnabled() const noexcept;
    bool IsUpdateble() const noexcept;
    bool IsShowMsgIdsEnabled() const noexcept;
    bool IsSaveCallRTPEnabled() const noexcept;
    bool IsServerSearchEnabled() const noexcept;
    bool IsShowHiddenThemes() const noexcept;
    bool IsSysCrashHandleEnabled() const noexcept;
    bool IsNetCompressionEnabled() const noexcept;
    bool IsSslVerificationEnabled() const noexcept;
    bool showSecondsInTimePicker() const noexcept;

    bool WatchGuiMemoryEnabled() const noexcept;

    /* Has the option changed during current app session? */
    bool ShowMsgOptionHasChanged() const noexcept;

    bool GDPR_UserHasAgreed() const noexcept;
    int32_t GDPR_AgreementReportedToServer() const noexcept;
    bool GDPR_AgreedButAwaitingSend() const noexcept;
    bool GDPR_UserHasLoggedInEver() const noexcept;

    const std::string& getMacUpdateAlpha() const noexcept;
    const std::string& getMacUpdateBeta() const noexcept;
    const std::string& getMacUpdateRelease() const noexcept;
    const std::string& getUrlAgentProfile() const noexcept;
    const std::string& getUrlCICQCom() const noexcept;
    const std::string& getUrlAttachPhone() const noexcept;
    const std::string& getDevId() const noexcept;

    /* seconds */
    std::chrono::seconds CacheHistoryControlPagesFor() const noexcept;
    std::chrono::seconds CacheHistoryControlPagesCheckInterval() const noexcept;
    uint32_t AppUpdateIntervalSecs() const noexcept;

    void SetFullLogEnabled(bool enabled) noexcept;
    void SetUpdateble(bool enabled) noexcept;
    void SetContextMenuFeaturesUnlocked(bool unlocked) noexcept;
    void SetShowMsgIdsEnabled(bool doShow) noexcept;
    void SetShowMsgOptionHasChanged(bool changed) noexcept;
    void SetSaveCallRTPEnabled(bool changed) noexcept;
    void SetServerSearchEnabled(bool enabled) noexcept;
    void SetShowHiddenThemes(bool enabled) noexcept;
    void SetGDPR_UserHasLoggedInEver(bool hasLoggedIn) noexcept;
    void SetGDPR_AgreementReportedToServer(GDPR_Report_To_Server_State state) noexcept;
    void SetCacheHistoryControlPagesFor(int secs) noexcept;
    void SetWatchGuiMemoryEnabled(bool _watch) noexcept;
    void SetCustomDeviceId(bool _custom) noexcept;
    void SetNetCompressionEnabled(bool _enabled) noexcept;
    void setShowSecondsInTimePicker(bool _enabled) noexcept;

    bool hasCustomDeviceId() const;

#ifdef IM_AUTO_TESTING
    QString toJsonString() const;
#endif

private:
    bool IsContextMenuFeaturesUnlocked_;
    bool IsServerHistoryEnabled_;
    bool IsCrashEnable_;
    bool IsTestingEnable_;
    bool IsFullLogEnabled_;
    bool IsUpdateble_;
    bool IsShowMsgIdsEnabled_;
    bool IsSaveCallRTPEnabled_;
    bool IsServerSearchEnabled_;
    bool IsShowHiddenThemes_;
    bool IsSysCrashHandleEnabled_;
    bool IsNetCompressionEnabled_;
    bool IsSslVerificationEnabled_;
    bool showSecondsInTimePicker_;

    bool WatchGuiMemoryEnabled_;

    bool ShowMsgOptionHasChanged_;

    bool GDPR_UserHasAgreed_;
    int32_t GDPR_AgreementReportedToServer_;
    bool GDPR_UserHasLoggedInEver_;

    int CacheHistoryContolPagesFor_;
    int CacheHistoryContolPagesCheckInterval_;

    uint32_t AppUpdateIntervalSecs_;

    std::string deviceId_;

    std::string urlMacUpdateAlpha_;
    std::string urlMacUpdateBeta_;
    std::string urlMacUpdateRelease_;
    std::string urlAttachPhone_;
};

typedef std::unique_ptr<AppConfig> AppConfigUptr;

const AppConfig& GetAppConfig();

void SetAppConfig(AppConfigUptr&& appConfig);
void ModifyAppConfig(AppConfig newAppConfig, message_processed_callback callback = nullptr, QObject *qobj = nullptr, bool postToCore = true);

UI_NS_END
