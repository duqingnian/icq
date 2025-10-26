#include "stdafx.h"

#include "../utils.h"

#ifndef STRIP_VOIP
#include "../Voip/VoipManager.h"
#endif
#include "stats/memory/memory_stats_collector.h"
#include "stats/memory/memory_stats_request.h"
#include "stats/memory/memory_stats_response.h"

#include "../tools/md5.h"
#include "../tools/system.h"
#include "../tools/file_sharing.h"

#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"

#include "../archive/contact_archive.h"
#include "../../corelib/enumerations.h"

#include "im_login.h"

// stickers
#include "../stickers/stickers.h"

#include "../themes/wallpaper_loader.h"

#include "base_im.h"
#include "../async_task.h"

using namespace core;

namespace fs = boost::filesystem;

namespace
{
    void __on_voip_user_bitmap(std::shared_ptr<voip_manager::VoipManager> vm, const std::string& contact, int type, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
    {
#ifndef STRIP_VOIP
        voip_manager::UserBitmap bmp;
        bmp.bitmap.data = (void*)data;
        bmp.bitmap.size = size;
        bmp.bitmap.w    = w;
        bmp.bitmap.h    = h;
        bmp.contact     = contact;
        bmp.type        = type;
        bmp.theme       = theme;

        vm->window_set_bitmap(bmp);
#endif
    }
}

base_im::base_im(const im_login_id& _login,
                 std::shared_ptr<voip_manager::VoipManager> _voip_manager,
                 std::shared_ptr<memory_stats::memory_stats_collector> _memory_stats_collector)
    :   voip_manager_(_voip_manager),
        id_(_login.get_id()),
        memory_stats_collector_(_memory_stats_collector)
{
}


base_im::~base_im()
{

}

void base_im::set_id(int32_t _id)
{
    id_ = _id;
}

int32_t base_im::get_id() const
{
    return id_;
}

std::shared_ptr<stickers::face> base_im::get_stickers()
{
    if (!stickers_)
        stickers_ = std::make_shared<stickers::face>(get_stickers_path());

    return stickers_;
}

std::shared_ptr<themes::wallpaper_loader> base_im::get_wallpaper_loader()
{
    if (!wp_loader_)
    {
        wp_loader_ = std::make_shared<themes::wallpaper_loader>();
        wp_loader_->clear_missing_wallpapers();
    }

    return wp_loader_;
}

std::vector<std::string> base_im::get_audio_devices_names()
{
    std::vector<std::string> result;

    if (voip_manager_)
    {
        std::vector<voip_manager::device_description> device_list;
        voip_manager_->get_device_list(DeviceType::AudioRecording, device_list);

        result.reserve(device_list.size());
        for (auto & device : device_list)
            result.push_back(device.name);
    }

    return result;
}

std::vector<std::string> base_im::get_video_devices_names()
{
    std::vector<std::string> result;

    if (voip_manager_)
    {
        std::vector<voip_manager::device_description> device_list;
        voip_manager_->get_device_list(DeviceType::VideoCapturing, device_list);

        result.reserve(device_list.size());
        for (auto & device : device_list)
        {
            if (device.videoCaptureType == VC_DeviceCamera)
                result.push_back(device.name);
        }
    }

    return result;
}

void base_im::connect()
{

}

std::wstring core::base_im::get_contactlist_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/contacts/cache.cl");
}

std::wstring core::base_im::get_my_info_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/info/cache");
}

std::wstring base_im::get_active_dialogs_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/dialogs/", archive::cache_filename());
}

std::wstring base_im::get_pinned_chats_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/favorites/", archive::cache_filename());
}

std::wstring base_im::get_unimportant_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/unimportant/", archive::cache_filename());
}

std::wstring base_im::get_mailboxes_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/mailboxes/", archive::cache_filename());
}

std::wstring core::base_im::get_im_data_path() const
{
    return su::wconcat(utils::get_product_data_path(), L'/', get_im_path());
}

std::wstring core::base_im::get_avatars_data_path() const
{
    return su::wconcat(get_im_data_path(), L"/avatars/");
}

std::wstring base_im::get_file_name_by_url(const std::string& _url) const
{
    return su::wconcat(get_content_cache_path(), L'/', core::tools::from_utf8(core::tools::md5(_url.data(), _url.size())));
}

std::wstring base_im::get_stickers_path() const
{
    return su::wconcat(get_im_data_path(), L"/stickers");
}

std::wstring base_im::get_im_downloads_path(const std::string &alt) const
{
#ifdef __linux__
    typedef std::wstring_convert<std::codecvt_utf8<wchar_t>> converter_t;
#else
    typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter_t;
#endif

    auto user_downloads_path = core::tools::system::get_user_downloads_dir();

    converter_t converter;
    const auto walt = converter.from_bytes(alt.c_str());
    if (!walt.empty())
    {
        user_downloads_path = walt;
    }

    if (!core::tools::system::is_dir_writable(user_downloads_path))
        user_downloads_path = core::tools::system::get_user_downloads_dir();

    if constexpr (platform::is_apple())
        return user_downloads_path;

    return su::wconcat(user_downloads_path, L'/', core::tools::from_utf8(config::get().string(config::values::product_name_full)));
}

std::wstring base_im::get_content_cache_path() const
{
    return su::wconcat(get_im_data_path(), L"/content.cache");
}

std::wstring core::base_im::get_last_suggest_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/smartreply/", archive::cache_filename());
}

std::wstring core::base_im::get_search_history_path() const
{
    return su::wconcat(get_im_data_path(), L"/search");
}

std::wstring core::base_im::get_call_log_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/calls/call_log.cache");
}

std::wstring core::base_im::get_inviters_blacklist_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/privacy/blocked_inviters.cache");
}

std::wstring core::base_im::get_tasks_file_name() const
{
    return su::wconcat(get_im_data_path(), L"/tasks/", archive::cache_filename());
}

// voip
/*void core::base_im::on_voip_call_set_proxy(const voip_manager::VoipProxySettings& proxySettings)
{
#ifndef STRIP_VOIP
    voip_manager_->call_set_proxy(proxySettings);
#endif
}*/

#ifndef STRIP_VOIP
void core::base_im::on_voip_call_start(const std::vector<std::string> &contacts, const voip_manager::CallStartParams &params)
{
    auto account = _get_protocol_uid();
    im_assert(!account.empty());
    if (!voip_manager_ || account.empty() || contacts.empty())
        return;
    voip_manager_->call_create(contacts, account, params);
    voip_manager_->set_device_mute(AudioPlayback, false);
}

void core::base_im::on_voip_user_update_avatar_no_video(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
#if 0
    __on_voip_user_bitmap(voip_manager_, contact, /*voip2::AvatarType_UserNoVideo*/0, data, size, h, w, theme);
#endif
}

void core::base_im::on_voip_user_update_avatar_text(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    __on_voip_user_bitmap(voip_manager_, contact, /*voip2::AvatarType_UserText*/Img_Username, data, size, h, w, theme);
}

void core::base_im::on_voip_user_update_avatar_text_header(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
#if 0
    __on_voip_user_bitmap(voip_manager_, contact, /*voip2::AvatarType_Header*/0, data, size, h, w, theme);
#endif
}

void core::base_im::on_voip_user_update_avatar(const std::string& contact, const unsigned char* data, unsigned size, unsigned avatar_h, unsigned avatar_w, voip_manager::AvatarThemeType theme)
{
    return __on_voip_user_bitmap(voip_manager_, contact, /*voip2::AvatarType_UserMain*/Img_Avatar, data, size, avatar_h, avatar_w, theme);
}

void core::base_im::on_voip_user_update_avatar_background(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme)
{
    return __on_voip_user_bitmap(voip_manager_, contact, /*voip2::AvatarType_Background*/Img_UserBackground, data, size, h, w, theme);
}

void core::base_im::on_voip_call_end(const std::string &call_id, const std::string &contact, bool busy, bool conference)
{
    voip_manager_->call_decline(voip_manager::Contact(call_id, contact), busy, conference);
}

void core::base_im::on_voip_device_changed(std::string_view dev_type, const std::string& uid, bool force_reset)
{
    if ("audio_playback" == dev_type)
    {
        voip_manager_->set_device(AudioPlayback, uid, force_reset);
    }
    else if ("audio_capture" == dev_type)
    {
        voip_manager_->set_device(AudioRecording, uid, force_reset);
    }
    else if ("video_capture" == dev_type)
    {
        voip_manager_->set_device(VideoCapturing, uid, force_reset);
    }
    else
    {
        im_assert(false);
    }
}

void core::base_im::on_voip_devices_changed(DeviceClass deviceClass)
{
    voip_manager_->notify_devices_changed(deviceClass);
}

void core::base_im::on_voip_call_accept(const std::string &call_id, bool video)
{
    voip_manager_->call_accept(call_id, _get_protocol_uid(), video);
    voip_manager_->set_device_mute(AudioPlayback, false);
}

void core::base_im::on_voip_add_window(voip_manager::WindowParams& windowParams)
{
    voip_manager_->window_add(windowParams);
}

void core::base_im::on_voip_remove_window(void* hwnd)
{
    voip_manager_->window_remove(hwnd);
}

void core::base_im::on_voip_call_stop()
{
    voip_manager_->call_stop();
    voip_manager_->set_device_mute(AudioPlayback, false);
}

void core::base_im::on_voip_switch_media(bool video)
{
    if (video)
    {
        const bool enabled = voip_manager_->local_video_enabled();
        voip_manager_->media_video_en(!enabled);
    }
    else
    {
        const bool enabled = voip_manager_->local_audio_enabled();
        voip_manager_->media_audio_en(!enabled);
    }
}

void core::base_im::on_voip_mute_incoming_call_sounds(bool mute)
{
#if defined(_WIN32)
    voip_manager_->mute_incoming_call_sounds(mute);
#endif
}

void core::base_im::on_voip_mute_switch()
{
    const bool mute = voip_manager_->get_device_mute(AudioPlayback);
    voip_manager_->set_device_mute(AudioPlayback, !mute);
}

void core::base_im::on_voip_set_mute(bool mute)
{
    voip_manager_->set_device_mute(AudioPlayback, mute);
}

void core::base_im::on_voip_reset()
{
    voip_manager_->reset();
}

void core::base_im::on_voip_proto_ack(const voip_manager::VoipProtoMsg& msg, bool success)
{
    voip_manager_->ProcessVoipAck(msg, success);
}

void core::base_im::on_voip_proto_msg(bool allocate, const char* data, unsigned len, std::shared_ptr<auto_callback> _on_complete)
{
    auto account = _get_protocol_uid();
    im_assert(!account.empty());
    if (!account.empty())
    {
        const auto msg_type = allocate ? 0 : 1;
        voip_manager_->ProcessVoipMsg(account, msg_type, data, len);
        _on_complete->callback(0);
    }
}

void core::base_im::on_voip_update()
{
    voip_manager_->update();
}

bool core::base_im::has_created_call()
{
    return voip_manager_->has_created_call();
}

void base_im::send_voip_calls_quality_report(int _score, const std::string &_survey_id,
                                             const std::vector<std::string> &_reasons,  const std::string& _aimid)
{
    voip_manager_->call_report_user_rating(voip_manager::UserRatingReport(_score, _survey_id, _reasons, _aimid));
}

void core::base_im::on_voip_peer_resolution_changed(const std::string& _contact, int _width, int _height)
{
    voip_manager_->peer_resolution_changed(_contact, _width, _height);
}
#endif
