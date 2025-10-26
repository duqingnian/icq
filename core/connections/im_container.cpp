#include "stdafx.h"
#include "im_container.h"
#include "login_info.h"
#include "im_login.h"
#include "wim/chat_params.h"
#include "../../corelib/collection_helper.h"
#include "../../corelib/enumerations.h"
#include "../archive/gallery_cache.h"
#include "../stats/memory/memory_stats_collector.h"
#include "wim/wim_im.h"
#include "wim/wim_packet.h"
#include "wim/auth_parameters.h"
#ifndef STRIP_VOIP
#include "../Voip/VoipManager.h"
#endif

#include "../core.h"
#include "../tools/system.h"
#include "../tools/file_sharing.h"
#include "../utils.h"
#include "../archive/contact_archive.h"
#include "../archive/history_message.h"
#include "../archive/thread_parent_topic.h"
#include "../smartreply/smartreply_suggest.h"
#include "../statistics.h"
#include "../proxy_settings.h"

#include "../configuration/app_config.h"
#include "../configuration/host_config.h"
#include "../configuration/external_config.h"
#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"

#include "tools/features.h"

namespace
{
    constexpr int VOIP_RATING_STARS_SCORE_COEFFICIENT = 10;

    std::vector<std::string> to_string_vector(core::coll_helper& _params, std::string_view _name)
    {
        std::vector<std::string> res;

        if (_params.is_value_exist(_name))
        {
            if (auto arr = _params.get_value_as_array(_name))
            {
                const auto size = arr->size();
                res.reserve(size);
                for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
                    res.emplace_back(arr->get_at(i)->get_as_string());
            }
        }
        return res;
    }
}

using namespace core;

voip_manager::BitmapDescription get_bitmap_from_stream(core::istream* stream, int w, int h)
{
    voip_manager::BitmapDescription res;
    res.size = stream->size();
    res.data = (void*)stream->read(res.size);
    res.w = w;
    res.h = h;

    return res;
}

im_container::im_container(const std::shared_ptr<voip_manager::VoipManager>& voip_manager,
                           const std::shared_ptr<memory_stats::memory_stats_collector>& memory_stats_collector)
    : logins_(std::make_unique<im_login_list>(utils::get_product_data_path() + L"/settings/ims.stg"))
    , voip_manager_(voip_manager),
      memory_stats_collector_(memory_stats_collector)
{
    REGISTER_IM_MESSAGE("login_by_password", on_login_by_password);
    REGISTER_IM_MESSAGE("login_get_sms_code", on_login_get_sms_code);
    REGISTER_IM_MESSAGE("login_by_phone", on_login_by_phone);
    REGISTER_IM_MESSAGE("login_by_oauth2", on_login_by_oauth2);
    REGISTER_IM_MESSAGE("logout", on_logout);
    REGISTER_IM_MESSAGE("connect_after_migration", on_connect_after_migration);
    REGISTER_IM_MESSAGE("avatars/get", on_get_contact_avatar);
    REGISTER_IM_MESSAGE("avatars/show", on_show_contact_avatar);
    REGISTER_IM_MESSAGE("avatars/remove", on_remove_contact_avatars);
    REGISTER_IM_MESSAGE("send_message", on_send_message);
    REGISTER_IM_MESSAGE("update_message", on_update_message);
    REGISTER_IM_MESSAGE("message/typing", on_message_typing);
    //REGISTER_IM_MESSAGE("set_state", on_set_state);
    REGISTER_IM_MESSAGE("archive/buddies/get", on_get_archive_messages_buddies);
    REGISTER_IM_MESSAGE("archive/messages/get", on_get_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete", on_delete_archive_messages);
    REGISTER_IM_MESSAGE("archive/messages/delete_from", on_delete_archive_messages_from);
    REGISTER_IM_MESSAGE("archive/messages/delete_all", on_delete_archive_all_messages);
    REGISTER_IM_MESSAGE("archive/mentions/get", on_get_archive_mentions);
    REGISTER_IM_MESSAGE("archive/log/model", on_archive_log_model);
    REGISTER_IM_MESSAGE("messages/context/get", on_get_message_context);
    REGISTER_IM_MESSAGE("archive/make/holes", on_make_archive_holes);
    REGISTER_IM_MESSAGE("archive/invalidate/history", on_invalidate_history);
    REGISTER_IM_MESSAGE("archive/invalidate/message_data", on_invalidate_archive_data);

    REGISTER_IM_MESSAGE("dialogs/search/local", on_dialogs_search_local);
    REGISTER_IM_MESSAGE("dialogs/search/local/end", on_dialogs_search_local_ended);
    REGISTER_IM_MESSAGE("dialogs/search/server", on_dialogs_search_server);
    REGISTER_IM_MESSAGE("dialogs/search/add_pattern", on_dialogs_search_add_pattern);
    REGISTER_IM_MESSAGE("dialogs/search/remove_pattern", on_dialogs_search_remove_pattern);

    REGISTER_IM_MESSAGE("thread/feed/search/local", on_thread_feed_search_local);
    REGISTER_IM_MESSAGE("threads/search/local", on_threads_search_local);
    REGISTER_IM_MESSAGE("threads/search/local/end", on_threads_search_local_ended);
    REGISTER_IM_MESSAGE("threads/search/server", on_threads_search_server);

    REGISTER_IM_MESSAGE("dialogs/add", on_add_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/remove", on_remove_opened_dialog);
    REGISTER_IM_MESSAGE("dialogs/hide", on_hide_chat);
    REGISTER_IM_MESSAGE("dialogs/mute", on_mute_chat);
    REGISTER_IM_MESSAGE("dialogs/set_attention_attribute", on_set_attention_attribute);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read", on_set_last_read);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read_mention", on_set_last_read_mention);
    REGISTER_IM_MESSAGE("dlg_state/set_last_read_partial", on_set_last_read_partial);
    REGISTER_IM_MESSAGE("dlg_state/close_stranger", on_close_stranger);
    REGISTER_IM_MESSAGE("files/upload", on_upload_file_sharing);
    REGISTER_IM_MESSAGE("files/upload/abort", on_abort_file_sharing_uploading);
    REGISTER_IM_MESSAGE("files/preview_size", on_get_file_sharing_preview_size);
    REGISTER_IM_MESSAGE("files/download/metainfo", on_download_file_sharing_metainfo);
    REGISTER_IM_MESSAGE("files/download", on_download_file);
    REGISTER_IM_MESSAGE("files/download/abort", on_abort_file_downloading);
    REGISTER_IM_MESSAGE("image/download", on_download_image);
    REGISTER_IM_MESSAGE("loader/download/cancel", on_cancel_loader_task);
    REGISTER_IM_MESSAGE("external_file_path/get", on_get_external_file_path);
    REGISTER_IM_MESSAGE("link_metainfo/download", on_download_link_preview);
    REGISTER_IM_MESSAGE("download/raise_priority", on_download_raise_priority);

    REGISTER_IM_MESSAGE("stickers/meta/get", on_get_stickers_meta);
    REGISTER_IM_MESSAGE("stickers/sticker/get", on_get_sticker);
    REGISTER_IM_MESSAGE("stickers/sticker/get/cancel", on_get_sticker_cancel);
    REGISTER_IM_MESSAGE("stickers/fs_by_ids/get", on_get_fs_stickers_by_ids);
    REGISTER_IM_MESSAGE("stickers/pack/info", on_get_stickers_pack_info);
    REGISTER_IM_MESSAGE("stickers/pack/add", on_add_stickers_pack);
    REGISTER_IM_MESSAGE("stickers/pack/remove", on_remove_stickers_pack);
    REGISTER_IM_MESSAGE("stickers/store/get", on_get_stickers_store);
    REGISTER_IM_MESSAGE("stickers/store/search", on_search_stickers_store);
    REGISTER_IM_MESSAGE("stickers/big_set_icon/get", on_get_set_icon_big);
    REGISTER_IM_MESSAGE("stickers/big_set_icon/clean", on_clean_set_icon_big);
    REGISTER_IM_MESSAGE("stickers/set/order", on_set_sticker_order);
    REGISTER_IM_MESSAGE("stickers/suggests/get", on_get_sticker_suggests);
    REGISTER_IM_MESSAGE("smartreply/suggests/clearall", on_clear_smartreply_suggests);
    REGISTER_IM_MESSAGE("smartreply/suggests/clear", on_clear_smartreply_suggests);
    REGISTER_IM_MESSAGE("smartreply/suggests/load", on_load_smartreply_suggests);
    REGISTER_IM_MESSAGE("smartreply/get", on_get_smartreplies);

    REGISTER_IM_MESSAGE("chats/info/get", on_get_chat_info);
    REGISTER_IM_MESSAGE("chats/home/get", on_get_chat_home);
    REGISTER_IM_MESSAGE("chats/pending/resolve", on_resolve_pending);
    REGISTER_IM_MESSAGE("chats/member/info", on_get_chat_member_info);
    REGISTER_IM_MESSAGE("chats/members/get", on_get_chat_members);
    REGISTER_IM_MESSAGE("chats/members/search", on_search_chat_members);
    REGISTER_IM_MESSAGE("chats/contacts/get", on_get_chat_contacts);
    REGISTER_IM_MESSAGE("contacts/search/server", on_search_contacts_server);
    REGISTER_IM_MESSAGE("contacts/search/local", on_search_contacts_local);
    REGISTER_IM_MESSAGE("contacts/add", on_add_contact);
    REGISTER_IM_MESSAGE("contacts/remove", on_remove_contact);
    REGISTER_IM_MESSAGE("contacts/rename", on_rename_contact);
    REGISTER_IM_MESSAGE("contacts/ignore", on_ignore_contact);
    REGISTER_IM_MESSAGE("contacts/get_ignore", on_get_ignore_contacts);
    REGISTER_IM_MESSAGE("contact/switched", on_contact_switched);
    REGISTER_IM_MESSAGE("dlg_states/hide", on_hide_dlg_state);
    REGISTER_IM_MESSAGE("add_members", on_add_members);
    REGISTER_IM_MESSAGE("remove_members", on_remove_members);
    REGISTER_IM_MESSAGE("stats", on_stats);
    REGISTER_IM_MESSAGE("im_stats", on_im_stats);
    REGISTER_IM_MESSAGE("addressbook/sync", on_syncronize_addressbook);

    REGISTER_IM_MESSAGE("themes/meta/check", on_check_theme_meta_updates);
    REGISTER_IM_MESSAGE("themes/wallpaper/get", on_get_theme_wallpaper);
    REGISTER_IM_MESSAGE("themes/wallpaper/preview/get", on_get_theme_wallpaper_preview);
    REGISTER_IM_MESSAGE("themes/wallpaper/user/set", on_set_user_wallpaper);
    REGISTER_IM_MESSAGE("themes/wallpaper/user/remove", on_set_user_wallpaper);

    REGISTER_IM_MESSAGE("files/speech_to_text", on_speech_to_text);
    REGISTER_IM_MESSAGE("pin_chat", on_pin_chat);
    REGISTER_IM_MESSAGE("unfavorite", on_unfavorite);
    REGISTER_IM_MESSAGE("update_profile", on_update_profile);
    REGISTER_IM_MESSAGE("mark_unimportant", on_mark_unimportant);
    REGISTER_IM_MESSAGE("remove_from_unimportant", on_remove_from_unimportant);
    REGISTER_IM_MESSAGE("set_user_proxy_settings", on_set_user_proxy);
    REGISTER_IM_MESSAGE("livechat/join", on_join_livechat);
    REGISTER_IM_MESSAGE("livechat/join/cancel", on_cancel_join_livechat);
    REGISTER_IM_MESSAGE("livechat/invite/cancel", on_cancel_chat_invitation);
    REGISTER_IM_MESSAGE("set_locale", on_set_locale);
    REGISTER_IM_MESSAGE("set_avatar", on_set_avatar);

    REGISTER_IM_MESSAGE("chats/create", on_create_chat);

    REGISTER_IM_MESSAGE("chats/mod/params", on_mod_chat_params);
    REGISTER_IM_MESSAGE("chats/mod/name", on_mod_chat_name);
    REGISTER_IM_MESSAGE("chats/mod/about", on_mod_chat_about);
    REGISTER_IM_MESSAGE("chats/mod/rules", on_mod_chat_rules);

    REGISTER_IM_MESSAGE("chats/block", on_block_chat_member);
    REGISTER_IM_MESSAGE("chats/role/set", on_set_chat_member_role);
    REGISTER_IM_MESSAGE("chats/message/pin", on_chat_pin_message);
    REGISTER_IM_MESSAGE("phoneinfo", on_phoneinfo);
    REGISTER_IM_MESSAGE("mrim/get_key", on_mrim_get_key);

    REGISTER_IM_MESSAGE("mentions/suggests/get", on_get_mentions_suggests);
    REGISTER_IM_MESSAGE("get_code_by_phone_call", on_get_code_by_phone_call);

    REGISTER_IM_MESSAGE("get_logs_path", on_get_logs_path);
    REGISTER_IM_MESSAGE("change_app_config", on_change_app_config);
    REGISTER_IM_MESSAGE("remove_content_cache", on_remove_content_cache);
    REGISTER_IM_MESSAGE("clear_avatars", on_clear_avatars);
    REGISTER_IM_MESSAGE("remove_ominron_stg", on_remove_omicron_stg);

    REGISTER_IM_MESSAGE("report/contact", on_report_contact);
    REGISTER_IM_MESSAGE("report/stickerpack", on_report_stickerpack);
    REGISTER_IM_MESSAGE("report/sticker", on_report_sticker);
    REGISTER_IM_MESSAGE("report/message", on_report_message);

    REGISTER_IM_MESSAGE("agreement/gdpr", on_user_accept_gdpr);

#ifndef STRIP_VOIP
    REGISTER_IM_MESSAGE("voip_call", on_voip_call_message);
    REGISTER_IM_MESSAGE("get_voip_calls_quality_popup_conf", on_get_voip_calls_quality_popup_conf);
    REGISTER_IM_MESSAGE("send_voip_calls_quality_report", on_send_voip_calls_quality_report);
#endif

    REGISTER_IM_MESSAGE("get_dialog_gallery", on_get_dialog_gallery);
    REGISTER_IM_MESSAGE("get_dialog_gallery_by_msg", on_get_dialog_gallery_by_msg);
    REGISTER_IM_MESSAGE("request_gallery_state", on_request_gallery_state);
    REGISTER_IM_MESSAGE("get_gallery_index", on_get_gallery_index);
    REGISTER_IM_MESSAGE("make_gallery_hole", on_make_gallery_hole);
    REGISTER_IM_MESSAGE("stop_gallery_holes_downloading", on_stop_gallery_holes_downloading);

    REGISTER_IM_MESSAGE("request_memory_usage", on_request_memory_usage);
    REGISTER_IM_MESSAGE("report_memory_usage", on_report_memory_usage);
    REGISTER_IM_MESSAGE("get_ram_usage", on_get_ram_usage);
    REGISTER_IM_MESSAGE("_ui_activity", on_ui_activity);

    REGISTER_IM_MESSAGE("localpin/set", on_local_pin_set);
    REGISTER_IM_MESSAGE("localpin/entered", on_local_pin_entered);
    REGISTER_IM_MESSAGE("localpin/disable", on_local_pin_disable);

    REGISTER_IM_MESSAGE("idinfo/get", on_get_id_info);

    REGISTER_IM_MESSAGE("update/check", on_update_check);

    REGISTER_IM_MESSAGE("get_id_info", on_get_id_info);
    REGISTER_IM_MESSAGE("get_user_info", on_get_user_info);
    REGISTER_IM_MESSAGE("get_user_last_seen", on_get_user_last_seen);

    REGISTER_IM_MESSAGE("privacy_settings/set", on_set_privacy_settings);
    REGISTER_IM_MESSAGE("privacy_settings/get", on_get_privacy_settings);

    REGISTER_IM_MESSAGE("nickname/check", on_nickname_check);
    REGISTER_IM_MESSAGE("group_nickname/check", on_group_nickname_check);
    REGISTER_IM_MESSAGE("common_chats/get", on_get_common_chats);

    REGISTER_IM_MESSAGE("connection/reset", on_reset_connection);

    REGISTER_IM_MESSAGE("recent_avatars_size/update", on_update_recent_avatars_size);
    REGISTER_IM_MESSAGE("set_install_beta_updates", on_set_install_beta_updates);

    REGISTER_IM_MESSAGE("poll/get", on_get_poll);
    REGISTER_IM_MESSAGE("poll/vote", on_vote_in_poll);
    REGISTER_IM_MESSAGE("poll/revoke", on_revoke_vote);
    REGISTER_IM_MESSAGE("poll/stop", on_stop_poll);

    REGISTER_IM_MESSAGE("task/create", on_create_task);
    REGISTER_IM_MESSAGE("task/edit", on_edit_task);
    REGISTER_IM_MESSAGE("task/request_initial", on_request_initial_tasks);
    REGISTER_IM_MESSAGE("task/update_last_used", on_task_update_last_used);

    REGISTER_IM_MESSAGE("group/subscribe", on_group_subscribe);
    REGISTER_IM_MESSAGE("group/cancel_subscription", on_group_cancel_subscription);

    REGISTER_IM_MESSAGE("group/invitebl/add", on_group_inviteblacklist_add);
    REGISTER_IM_MESSAGE("group/invitebl/remove", on_group_inviteblacklist_remove);
    REGISTER_IM_MESSAGE("group/invitebl/search", on_group_inviteblacklist_search);
    REGISTER_IM_MESSAGE("group/invitebl/cl", group_blacklisted_cl_inviters);

    REGISTER_IM_MESSAGE("suggest_group_nick", on_suggest_group_nick);

    REGISTER_IM_MESSAGE("get_bot_callback_answer", on_get_bot_callback_answer);
    REGISTER_IM_MESSAGE("bot/start", on_start_bot);

    REGISTER_IM_MESSAGE("conference/create", on_create_confrence);

    REGISTER_IM_MESSAGE("sessions/get", on_get_sessions);
    REGISTER_IM_MESSAGE("sessions/reset", on_reset_session);
    REGISTER_IM_MESSAGE("sessions/resetall", on_reset_session);

    REGISTER_IM_MESSAGE("reaction/get", on_get_reactions);
    REGISTER_IM_MESSAGE("reaction/add", on_add_reaction);
    REGISTER_IM_MESSAGE("reaction/remove", on_remove_reaction);
    REGISTER_IM_MESSAGE("reaction/list", on_list_reactions);

    REGISTER_IM_MESSAGE("status/set", on_status_set);

    REGISTER_IM_MESSAGE("subscribe/status", on_subscribe_status);
    REGISTER_IM_MESSAGE("unsubscribe/status", on_unsubscribe_status);
    REGISTER_IM_MESSAGE("subscribe/call_room_info", on_subscribe_call_room_info);
    REGISTER_IM_MESSAGE("unsubscribe/call_room_info", on_unsubscribe_call_room_info);
    REGISTER_IM_MESSAGE("subscribe/thread", on_subscribe_thread_events);
    REGISTER_IM_MESSAGE("unsubscribe/thread", on_unsubscribe_thread_events);
    REGISTER_IM_MESSAGE("subscribe/task", on_subscribe_task);
    REGISTER_IM_MESSAGE("unsubscribe/task", on_unsubscribe_task);
    REGISTER_IM_MESSAGE("subscribe/filesharing_antivirus", on_subscribe_filesharing_antivirus);
    REGISTER_IM_MESSAGE("unsubscribe/filesharing_antivirus", on_unsubscribe_filesharing_antivirus);
    REGISTER_IM_MESSAGE("subscribe/mails_counter", on_subscribe_mails_counter);
    REGISTER_IM_MESSAGE("unsubscribe/mails_counter", on_unsubscribe_mails_counter);
    REGISTER_IM_MESSAGE("subscribe/tasks_counter", on_subscribe_tasks_counter);
    REGISTER_IM_MESSAGE("unsubscribe/tasks_counter", on_unsubscribe_tasks_counter);

    REGISTER_IM_MESSAGE("emoji/get", on_get_emoji);

    REGISTER_IM_MESSAGE("send_notify_sms", on_send_notify_sms);

    REGISTER_IM_MESSAGE("threads/add", on_add_thread);
    REGISTER_IM_MESSAGE("threads/get", on_get_thread);
    REGISTER_IM_MESSAGE("threads/feed/get", on_threads_feed_get);
    REGISTER_IM_MESSAGE("threads/subscribe", on_subscribe_thread);
    REGISTER_IM_MESSAGE("threads/unsubscribe", on_unsubscribe_thread);
    REGISTER_IM_MESSAGE("threads/subscribers/get", on_threads_subscribers_get);
    REGISTER_IM_MESSAGE("threads/subscribers/search", on_threads_subscribers_search);
    REGISTER_IM_MESSAGE("thread/autosubscribe", on_thread_autosubscribe);

    REGISTER_IM_MESSAGE("chats/thread/add", on_chats_thread_add);
    REGISTER_IM_MESSAGE("chats/thread/remove", on_chats_thread_remove);


    REGISTER_IM_MESSAGE("draft/set", on_draft_set);
    REGISTER_IM_MESSAGE("draft/get", on_draft_get);

    REGISTER_IM_MESSAGE("miniapp/start_session", on_miniapp_start_session);
}


im_container::~im_container()
{
}

void core::im_container::create()
{
    if (create_ims())
        connect_ims();
}

void im_container::connect_ims()
{
    for (const auto& x : ims_)
        x->connect();
}

bool im_container::update_login(im_login_id& _login)
{
    return logins_->update(_login);
}

void im_container::replace_uin_in_login(im_login_id& old_login, im_login_id& new_login)
{
    logins_->replace_uin(old_login, new_login);
}

bool core::im_container::create_ims()
{
    if (logins_->load())
    {
    }

    im_login_id login(std::string(), default_im_id);

    if (logins_->get_first_login(login))
    {
        // There has been a login at some point
        set_user_has_been_logged_in_ever(true);
    }
    else
    {
        const auto app_ini_path = utils::get_app_ini_path();
        configuration::dump_app_config_to_disk(app_ini_path);
    }

    ims_.push_back(std::make_shared<wim::im>(login, voip_manager_, memory_stats_collector_));

    return !ims_.empty();
}

std::string core::im_container::get_first_login() const
{
    im_login_id login(std::string(), default_im_id);
    logins_->get_first_login(login);
    return login.get_login();
}

void core::im_container::download_file(priority_t _priority, const std::string& _file_url, const std::string& _destination,
                                       std::string_view _normalized_url, bool _is_binary_data, std::function<void(bool)> _on_result)
{
    auto im = get_default_im();
    if (!im)
        return;

    im->download_file(_priority, _file_url, _destination, _normalized_url, _is_binary_data, std::move(_on_result));
}

void core::im_container::on_message_from_gui(std::string_view _message, int64_t _seq, coll_helper& _params)
{
    if (const auto iter_handler = messages_map_.find(_message); iter_handler != messages_map_.end())
    {
        if (iter_handler->second)
            iter_handler->second(_seq, _params);
    }
    else
    {
        im_assert(!"unknown message type");
    }
}

#ifndef STRIP_VOIP

void core::im_container::on_voip_call_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    const std::string_view type = _params.get_value_as_string("type");
    if (!im && type != "voip_reset")
        return;

    if (type == "voip_call_start")
    {
        const auto contacts = to_string_vector(_params, "contacts");

        im_assert(!contacts.empty());
        if (contacts.empty())
            return;

        /*core::proxy_settings proxySettings = g_core->get_proxy_settings();
        voip_manager::VoipProxySettings voipProxySettings;
        fromInternalProxySettings2Voip(proxySettings, voipProxySettings);
        im->on_voip_call_set_proxy(voipProxySettings);*/
        const std::string_view call_type = _params.get_value_as_string("call_type");
        voip_manager::CallStartParams params;
        if (call_type == "vcs")
        {
            params.is_vcs = true;
        } else if (call_type == "pinned_room")
        {
            params.is_pinned_room = true;
            params.video  = _params.get_value_as_bool("video");
        } else
        {
            params.video  = _params.get_value_as_bool("video");
            params.attach = _params.get_value_as_bool("attach");
        }
        im->on_voip_call_start(contacts, params);
    }
    else if (type == "voip_add_window") {
        voip_manager::WindowParams windowParams = {};
        windowParams.hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        windowParams.call_id = _params.get_value_as_string("call_id");
        windowParams.isPrimary = _params.get_value_as_bool("mode");
        windowParams.isIncoming = _params.get_value_as_bool("incoming_mode");
        windowParams.scale = (float)_params.get_value_as_double("scale");

        std::vector<core::istream*> steramsForFree;

        // Function to get bitmap from params.
        auto getBitmapDescription = [&_params, &steramsForFree](const std::string& prefix, voip_manager::BitmapDescription& outBitmap) -> bool {
            bool res = false;
            if (_params.is_value_exist(prefix)) {
                core::istream* statusStream = _params.get_value_as_stream(prefix);
                const auto h = _params.get_value_as_int((prefix + "_height"));
                const auto w = _params.get_value_as_int((prefix + "_width"));
                im_assert(statusStream);
                if (statusStream) {
                    const auto stream_size = statusStream->size();
                    im_assert(stream_size);
                    if (stream_size > 0 && h > 0 && w > 0) {
                        outBitmap = get_bitmap_from_stream(statusStream, w, h);
                        res = true;
                    }

                    steramsForFree.push_back(statusStream);
                }
            }

            return res;
        };

        if (getBitmapDescription("camera_status", windowParams.cameraStatus.bitmap))
        {
            windowParams.cameraStatus.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("calling", windowParams.calling.bitmap))
        {
            windowParams.calling.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("closedByBusy", windowParams.closedByBusy.bitmap))
        {
            windowParams.calling.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("watermark", windowParams.watermark.bitmap))
        {
            windowParams.watermark.hwnd = windowParams.hwnd;
        }

        if (getBitmapDescription("connecting_status", windowParams.connecting.bitmap))
        {
            windowParams.connecting.hwnd = windowParams.hwnd;
        }

        im->on_voip_add_window(windowParams);

        for (auto stream : steramsForFree)
        {
            if (stream) {
                stream->reset();
            }
        }
    }
    else if (type == "voip_remove_window") {
        void* hwnd = (void*)(uintptr_t)_params.get_value_as_int64("handle");
        im->on_voip_remove_window(hwnd);
    }
    else if (type == "voip_call_stop") {
        im->on_voip_call_stop();
    }
    else if (type == "audio_playback_mute_switch") {
        im->on_voip_mute_switch();
    }
    else if (type == "voip_call_media_switch") {
        const bool video = _params.get_value_as_bool("video");
        im->on_voip_switch_media(video);
    }
    else if (type == "voip_sounds_mute") {
        const bool mute = _params.get_value_as_bool("mute");
        im->on_voip_mute_incoming_call_sounds(mute);
    }
    else if (type == "voip_call_decline") {
        const std::string_view mode = _params.get_value_as_string("mode");
        const std::string_view conference = _params.get_value_as_string("conference");
        const std::string call_id = _params.get_value_as_string("call_id");
        const std::string contact = _params.get_value_as_string("contact");
        im->on_voip_call_end(call_id, contact, mode == "busy", conference == "true");
    }
    else if (type == "converted_avatar") {
        on_voip_avatar_msg(im, _params);
    }
    else if (type == "voip_call_accept") {
        const std::string_view mode = _params.get_value_as_string("mode");
        std::string call_id = _params.get_value_as_string("call_id");

        im_assert(!call_id.empty());
        if (call_id.empty())
            return;
        /*core::proxy_settings proxySettings = g_core->get_proxy_settings();
        voip_manager::VoipProxySettings voipProxySettings;
        fromInternalProxySettings2Voip(proxySettings, voipProxySettings);
        im->on_voip_call_set_proxy(voipProxySettings);*/
        im->on_voip_call_accept(std::move(call_id), mode == "video");
    }
    else if (type == "device_change") {
        const std::string_view dev_type = _params.get_value_as_string("dev_type");
        const std::string uid = _params.get_value_as_string("uid");
        bool force_reset = _params.get_value_as_bool("force_reset");
        im->on_voip_device_changed(dev_type, uid, force_reset);
    }
    else if (type == "voip_devices_changed") {
        bool audio = _params.get_value_as_bool("audio");
        im->on_voip_devices_changed(audio ? Audio : Video);
    }
    else if (type == "update") {
        im->on_voip_update();
    }
    else if (type == "voip_reset") {
        if (!im) {
#ifndef STRIP_VOIP
            voip_manager_->reset();
#endif
        }
        else {
            im->on_voip_reset();
        }
    }
    else if (type == "audio_playback_mute")
    {
        const std::string mode = _params.get_value_as_string("mute");
        im->on_voip_set_mute(mode == "on");
    }
    else if (type == "voip_peer_resolution_changed")
    {
        const std::string contact = _params.get_value_as_string("contact");
        int width  = _params.get_value_as_int("width");
        int height = _params.get_value_as_int("height");
        im->on_voip_peer_resolution_changed(contact, width, height);
    }
    else
    {
        im_assert(false);
    }
}

void core::im_container::on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params) {
    typedef void (base_im::*__loader_func)(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);

    auto __load_avatar = [&_params, &im](const std::string& prefix, __loader_func func) {

        const voip_manager::AvatarThemeType theme = (voip_manager::AvatarThemeType)_params.get_value_as_int("theme");

        const std::string contact = _params.get_value_as_string("contact");
        const auto size = _params.get_value_as_int("size");// (!miniWindow ? _params.get_value_as_int("size") : _params.get_value_as_int("width"));

        im_assert(!contact.empty() && size);
        if (contact.empty() || !size) {
            return;
        }

        im_assert(!!im);
        if (!im) {
            return;
        }

        if (!_params.is_value_exist((prefix + "avatar").c_str())) {
            return;
        }

        core::istream* stream = _params.get_value_as_stream((prefix + "avatar").c_str());
        const auto h = _params.get_value_as_int((prefix + "avatar_height").c_str());
        const auto w = _params.get_value_as_int((prefix + "avatar_width").c_str());

        im_assert(stream);
        if (stream) {
            const auto stream_size = stream->size();

            im_assert(stream_size);
            if (stream_size > 0) {
                core::base_im& ptr = *im.get();
                (ptr.*func)(contact, stream->read(stream->size()), stream_size, h, w, theme);
                stream->reset();
            }
        }
    };

    __load_avatar("", &base_im::on_voip_user_update_avatar);
    __load_avatar("rem_camera_offline_", &base_im::on_voip_user_update_avatar_no_video);
    __load_avatar("sign_normal_", &base_im::on_voip_user_update_avatar_text);
    __load_avatar("sign_header_", &base_im::on_voip_user_update_avatar_text_header);
    //__load_avatar("loc_camera_offline_",  &base_im::on_voip_user_update_avatar_camera_off);
    //__load_avatar("loc_camera_disabled_", &base_im::on_voip_user_update_avatar_no_camera);
    __load_avatar("background_", &base_im::on_voip_user_update_avatar_background);
}

void im_container::on_get_voip_calls_quality_popup_conf(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);

    if (im)
        im->get_voip_calls_quality_popup_conf(_seq, _params.get_value_as_string("locale"), _params.get_value_as_string("lang"));
}

void im_container::on_send_voip_calls_quality_report(const int64_t _seq, coll_helper& _params)
{
    UNUSED_ARG(_seq);

    auto im = get_im(_params);
    if (im)
    {
        const int score = VOIP_RATING_STARS_SCORE_COEFFICIENT * _params.get_value_as_int("starsCount", -1);
        const auto reasons = to_string_vector(_params, "reasons");
        im->send_voip_calls_quality_report(score,
            _params.get_value_as_string("survey_id"),
            reasons,
            _params.get_value_as_string("aimid"));
    }
}

#endif

void im_container::on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::archive::msgids_list ids;
    if (auto idsArray = _params.get_value_as_array("ids"))
    {
        const auto size = idsArray->size();
        ids.reserve(size);
        for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
            ids.push_back(idsArray->get_at(i)->get_as_int64());
    }

    const auto is_updated = _params.get_value_as_bool("updated", false) ? core::wim::is_updated_messages::yes : core::wim::is_updated_messages::no;

    im->get_archive_messages_buddies(_seq, _params.get_value_as_string("contact"), std::make_shared<core::archive::msgids_list>(std::move(ids)), is_updated);
}


void im_container::on_get_archive_messages(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_archive_messages(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_int64("from"),
        _params.get_value_as_int64("count_early"),
        _params.get_value_as_int64("count_later"),
        _params.get_value_as_bool("need_prefetch"),
        _params.get_value_as_bool("is_first_request"),
        _params.get_value_as_bool("after_search"));
}

void core::im_container::on_get_archive_mentions(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mentions(
        _seq,
        _params.get_value_as_string("contact"));
}

void core::im_container::on_message_typing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto id = _params.get_value_as_string("id", "");
    im->send_message_typing(_seq, _params.get_value_as_string("contact"), (core::typing_status)_params.get_value_as_int("status"), id);
}

static core::archive::message_pack make_pack(const coll_helper& _params)
{
    std::string message;
    if (_params->is_value_exist("message"))
        message = _params.get_value_as_string("message");

    core::archive::format_data message_format;
    if (_params->is_value_exist("message_format"))
        message_format.unserialize(_params, "message_format");

    if (core::configuration::get_app_config().is_crash_enabled() && message == "!crash")
    {
        throw new std::runtime_error("artificial crash");
    }

    bool is_sms = false;
    auto type = message_type::base;
    if (_params->is_value_exist("is_sms"))
    {
        is_sms = _params.get_value_as_bool("is_sms");
        type = message_type::sms;
    }

    bool is_sticker = false;
    if (_params->is_value_exist("sticker"))
    {
        is_sticker = _params.get_value_as_bool("sticker");
        if (is_sticker)
        {
            std::stringstream ss_message;
            ss_message << "ext:" << _params.get_value_as_int("sticker/set_id") << ":sticker:" << _params.get_value_as_int("sticker/id");

            message = ss_message.str();
        }

        type = message_type::sticker;
    }

    core::archive::quotes_vec quotes;
    if (_params->is_value_exist("quotes"))
    {
        core::iarray* array = _params.get_value_as_array("quotes");
        const auto size = array->size();
        quotes.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::archive::quote q;
            q.unserialize(array->get_at(i)->get_as_collection());
            quotes.push_back(std::move(q));
        }
    }

    core::archive::mentions_map mentions;
    if (_params->is_value_exist("mentions"))
    {
        const core::iarray* array = _params.get_value_as_array("mentions");
        for (auto i = 0, size = array->size(); i < size; ++i)
        {
            auto coll = array->get_at(i)->get_as_collection();
            coll_helper helper(coll, false);
            mentions.emplace(helper.get_value_as_string("aimid"), helper.get_value_as_string("friendly"));
        }
    }

    std::string internal_id;
    if (_params->is_value_exist("internal_id"))
        internal_id = _params.get_value_as_string("internal_id");

    uint64_t time = 0;
    if (_params->is_value_exist("msg_time"))
        time = _params.get_value_as_int64("msg_time");

    im_assert(!(is_sms && is_sticker));

    std::string description;
    if (_params.is_value_exist("description"))
        description = _params.get_value_as_string("description");

    core::archive::format_data description_format;
    if (_params->is_value_exist("description_format"))
        description_format.unserialize(_params, "description_format");

    std::string url;
    if (_params.is_value_exist("url"))
        url = _params.get_value_as_string("url");

    common::tools::patch_version version;
    if (_params.is_value_exist("update_patch_version"))
        version.set_version(_params.get_value_as_string("update_patch_version"));

    if (_params.is_value_exist("offline_version"))
        version.set_offline_version(_params.get_value_as_int("offline_version"));

    core::archive::shared_contact shared_contact;
    if (_params.is_value_exist("shared_contact"))
    {
        core::archive::shared_contact_data contact;
        core::icollection* coll = _params.get_value_as_collection("shared_contact");
        if (contact.unserialize(coll))
            shared_contact = std::move(contact);
    }

    core::archive::geo geo;
    if (_params.is_value_exist("geo"))
    {
        core::archive::geo_data geo_data;
        core::icollection* coll = _params.get_value_as_collection("geo");
        if (geo_data.unserialize(coll))
            geo = std::move(geo_data);
    }

    core::archive::poll poll;
    if (_params.is_value_exist("poll"))
    {
        core::archive::poll_data poll_data;
        core::icollection* coll = _params.get_value_as_collection("poll");
        if (poll_data.unserialize(coll))
            poll = std::move(poll_data);
    }

    core::tasks::task task;
    if (_params.is_value_exist("task"))
    {
        core::tasks::task_data task_data;
        core::icollection* coll = _params.get_value_as_collection("task");
        if (task_data.unserialize(coll))
            task = std::move(task_data);
    }

    smartreply::marker_opt marker;
    if (_params.is_value_exist("smartreply"))
    {
        smartreply::marker marker_data;
        core::icollection* coll = _params.get_value_as_collection("smartreply");
        coll_helper helper(coll, false);
        if (marker_data.unserialize(helper))
            marker = std::move(marker_data);
    }

    std::string chat_sender;
    if (_params.is_value_exist("chat_sender"))
        chat_sender = _params.get_value_as_string("chat_sender");

    core::archive::message_pack pack;
    pack.message_ = std::move(message);
    pack.message_format_ = std::move(message_format);
    pack.quotes_ = std::move(quotes);
    pack.mentions_ = std::move(mentions);
    pack.internal_id_ = std::move(internal_id);
    pack.type_ = type;
    pack.message_time_ = time;
    pack.description_ = std::move(description);
    pack.description_format_ = std::move(description_format);
    pack.url_ = std::move(url);
    pack.version_ = std::move(version);
    pack.shared_contact_ = std::move(shared_contact);
    pack.geo_ = std::move(geo);
    pack.poll_ = std::move(poll);
    pack.task_ = std::move(task);
    pack.smartreply_marker_ = std::move(marker);
    pack.chat_sender_ = chat_sender;

    if (_params.is_value_exist("draft_delete_time"))
        pack.draft_delete_time_.emplace(_params.get_value_as_int64("draft_delete_time"));

    return pack;
}

void core::im_container::on_send_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (_params.is_value_exist("contact"))
    {
        im->send_message_to_contact(
            _seq,
            _params.get_value_as_string("contact"),
            make_pack(_params));
    }
    else if (_params->is_value_exist("contacts"))
    {
        const auto contacts_to_send = to_string_vector(_params, "contacts");
        im->send_message_to_contacts(_seq, contacts_to_send, make_pack(_params));
    }
}

void core::im_container::on_update_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string contact = _params.get_value_as_string("contact");
    const int64_t msgid = _params.get_value_as_int64("id", -1);

    auto pack = make_pack(_params);

    if (msgid != -1)
    {
        pack.internal_id_.clear();
    }

    im->update_message_to_contact(
        _seq,
        msgid,
        contact,
        pack);
}

void core::im_container::on_phoneinfo(int64_t seq, coll_helper& _params)
{
    std::shared_ptr<base_im> im = get_im(_params);
    if (!im)
    {
        im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);
        ims_.push_back(im);
    }

    std::string phone = _params.get_value_as_string("phone");
    std::string gui_locale = _params.get_value_as_string("gui_locale");

    im->phoneinfo(seq, phone, gui_locale);
}

void core::im_container::on_set_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto state = profile_state::online;
    std::string sstate = _params.get_value_as_string("state");
    if (sstate == "dnd")
        state = profile_state::dnd;
    else if (sstate == "away")
        state = profile_state::away;
    else if (sstate == "invisible")
        state = profile_state::invisible;
    else if (sstate == "offline")
        state = profile_state::offline;

    im->set_state(_seq, state);
}

void core::im_container::on_add_members(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->add_members(
            _params.get_value_as_string("aimid"),
            to_string_vector(_params, "members"),
            _params.get_value_as_bool("unblock"));
}

void core::im_container::on_remove_members(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->remove_members(
            _params.get_value_as_string("aimid"),
            to_string_vector(_params, "members"));
}

void core::im_container::on_mrim_get_key(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mrim_key(
        _seq,
        _params.get_value_as_string("email"));
}

void core::im_container::on_login_by_password(int64_t _seq, coll_helper& _params)
{
    login_info info;
    info.set_login_type(login_type::lt_login_password);
    info.set_login(_params.get_value_as_string("login"));
    info.set_password(_params.get_value_as_string("password"));
    info.set_save_auth_data(_params.get_value_as_bool("save_auth_data"));
    info.set_token_type(_params.get_value_as_enum<token_type>("token_type", token_type::basic));

    auto do_login = [_seq, wr_this = weak_from_this()](login_info&& _info)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        std::shared_ptr<base_im> im = std::make_shared<wim::im>(im_login_id(std::string()), ptr_this->voip_manager_, ptr_this->memory_stats_collector_);
        ptr_this->ims_.clear();
        im->login(_seq, _info);
        ptr_this->ims_.emplace_back(std::move(im));
    };

    auto login_error = [](ext_url_config_error _err)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", int(_err));
        g_core->post_message_to_gui("external_url_config/error", 0, coll.get());
    };

    if (config::get().is_on(config::features::external_url_config) && info.get_token_type() == token_type::otp_via_email)
    {
        const auto& login_str = info.get_login();
        const auto login = std::string_view(login_str);

        if (const auto at_idx = login.find('@'); at_idx != std::string::npos)
        {
            std::string_view host, login_domain;
            std::vector<std::string> urls;

            if (const auto hash_idx = login.find('#'); hash_idx != std::string::npos && hash_idx > at_idx)
            {
                const auto domain_pos = at_idx + 1;
                login_domain = login.substr(domain_pos, hash_idx - domain_pos);
                host = login.substr(hash_idx + 1);

                info.set_login(login_str.substr(0, hash_idx));

                urls.emplace_back(config::hosts::external_url_config::make_url_auto_preset(login_domain, host, true));
                urls.emplace_back(config::hosts::external_url_config::make_url_auto_preset(login_domain, su::concat("u.", host), true));
            }
            else
            {
                login_domain = login.substr(at_idx + 1);
                host = login_domain;

                if (auto url = config::hosts::external_url_config::make_url_preset(login_domain); !url.empty())
                {
                    urls.emplace_back(std::move(url));
                }
                else
                {
                    urls.emplace_back(config::hosts::external_url_config::make_url_auto_preset(login_domain, host));
                    urls.emplace_back(config::hosts::external_url_config::make_url_auto_preset(login_domain, su::concat("u.", host)));
                }
            }

            if (!host.empty())
            {
                config::hosts::load_external_url_config(std::move(urls), [do_login, login_error, isUserAgreementAccepted = _params.get_value_as_bool("user_agreement_accepted"), info = std::move(info)](ext_url_config_error _err, std::string_view _url) mutable
                {
                    if (_err == ext_url_config_error::ok)
                    {
                        im_assert(!_url.empty());
                        g_core->set_external_config_url(_url);
                        config::hosts::send_config_to_gui();

                        if (features::is_oauth2_login_allowed() && !config::hosts::get_host_url(config::hosts::host_url_type::oauth_url).empty())
                            info.set_login_type(login_type::lt_oauth2);

                        if (config::get().is_on(config::features::external_user_agreement) && !isUserAgreementAccepted)
                        {
                            coll_helper coll(g_core->create_collection(), true);
                            coll.set_value_as_string("terms_of_use_url", config::hosts::get_host_url(config::hosts::host_url_type::terms_of_use_url));
                            coll.set_value_as_string("privacy_policy_url", config::hosts::get_host_url(config::hosts::host_url_type::privacy_policy_url));
                            g_core->post_message_to_gui("need_external_user_agreement", 0, coll.get());
                        }
                        else
                        {
                            do_login(std::move(info));
                        }
                    }
                    else
                    {
                        login_error(_err);
                    }
                });
                return;
            }
            else
            {
                login_error(core::ext_url_config_error::config_host_invalid);
                return;
            }
        }
        else
        {
            login_error(core::ext_url_config_error::config_host_invalid);
            return;
        }
    }
    else
    {
        const auto& login = info.get_login();
        if (const auto at_idx = login.find('@'); at_idx != std::string::npos)
        {
            if (const auto hash_idx = login.find('#'); hash_idx != std::string::npos && hash_idx > at_idx)
            {
                info.set_login(login.substr(0, hash_idx));
            }
        }
    }

    do_login(std::move(info));
}

void core::im_container::on_login_get_sms_code(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));

    std::shared_ptr<base_im> im;

    auto is_login = _params.get_value_as_bool("is_login");
    if (is_login)
    {
        ims_.clear();
        im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);
    }
    else
    {
        im = get_im(_params);
    }

    im->login_normalize_phone(_seq, _params.get_value_as_string("country"), _params.get_value_as_string("phone"), _params.get_value_as_string("locale"), is_login);

    if (is_login)
        ims_.push_back(im);
}

void core::im_container::on_login_by_phone(int64_t _seq, coll_helper& _params)
{
    phone_info info;
    info.set_phone(_params.get_value_as_string("phone"));
    info.set_sms_code(_params.get_value_as_string("sms_code"));

    if (ims_.empty())
    {
        im_assert(!"ims empty");
        return;
    }

    bool is_login = _params.get_value_as_bool("is_login");
    if (is_login)
        (*ims_.begin())->login_by_phone(_seq, info);
    else
        (*ims_.begin())->start_attach_phone(_seq, info);
}

void core::im_container::on_login_by_oauth2(int64_t _seq, coll_helper& _params)
{
    ims_.clear();
    std::shared_ptr<base_im> im = std::make_shared<wim::im>(im_login_id(std::string()), voip_manager_, memory_stats_collector_);

    im->login_by_oauth2(_seq, _params.get_value_as_string("authcode"));
    ims_.push_back(im);
}

void core::im_container::on_connect_after_migration(int64_t _seq, coll_helper& _params)
{
    ims_.clear();
    create();
}

void core::im_container::on_logout(int64_t _seq, coll_helper& _params)
{
    im_assert(!ims_.empty());
    auto im = get_im(_params);
    if (!im)
        return;

    std::weak_ptr<base_im> wr_im = im;
    std::string aimid = im->get_login();
    auto __onlogout = [wr_this = weak_from_this(), wr_im, clear_data = !_params.get() || _params.get_value_as_bool("clear"), aimid = std::move(aimid)]() mutable
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->start_session_stats(false /* delayed */);

        if (auto ptr_im = wr_im.lock())
            ptr_im->erase_auth_data();

        g_core->set_local_pin_enabled(false);

        ptr_this->ims_.clear();

        g_core->execute_core_context({ [clear_data, aimid = std::move(aimid)]() // postpone deleting to avoid race with im dtor
        {
            if (clear_data)
                g_core->remove_local_data(aimid);
            g_core->update_gui_settings();

            g_core->post_message_to_gui("session_finished", 0, nullptr);
        } });
    };

    g_core->post_message_to_gui("need_login", 0, nullptr);

#ifndef STRIP_VOIP
    if (voip_manager_->call_get_count())
#else
    if (0)
#endif
    {
        auto __doLogout = [wr_im, __onlogout]()
        {
            if (auto ptr_im = wr_im.lock())
                ptr_im->logout(__onlogout);
        };
#ifndef STRIP_VOIP
        voip_manager_->call_stop_smart(__doLogout);
#endif //STRIP_VOIP
    }
    else
    {
        im->logout(__onlogout);
    }
}

void core::im_container::logout(const bool _is_auth_error)
{
    auto logout = [wr_this = weak_from_this(), _is_auth_error]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        im_assert(!ptr_this->ims_.empty());
        if (ptr_this->ims_.empty())
            return;

        ptr_this->ims_.front()->erase_auth_data();
        g_core->set_local_pin_enabled(false);

        coll_helper coll(g_core->create_collection(), true);

        coll.set_value_as_bool("is_auth_error", _is_auth_error);

        g_core->post_message_to_gui("need_login", 0, coll.get());

        ptr_this->ims_.clear();

        if (config::get().is_on(config::features::external_url_config))
            config::hosts::clear_external_config();

        g_core->post_message_to_gui("session_finished", 0, nullptr);
    };

#ifndef STRIP_VOIP
    voip_manager_->call_stop_smart(logout);
#else
    logout();
#endif //!STRIP_VOIP
}

void core::im_container::on_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    if (const auto prop_array = _params.get_value_as_array("props"))
    {
        const auto size = prop_array->size();
        props.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
            const auto prop_name = value.get_value_as_string("name");
            const auto prop_value = value.get_value_as_string("value");
            props.emplace_back(prop_name, prop_value);
        }
    }

    g_core->insert_event((core::stats::stats_event_names)_params.get_value_as_int("event"), std::move(props));
}

void core::im_container::on_im_stats(int64_t _seq, coll_helper& _params)
{
    core::stats::event_props_type props;

    if (const auto prop_array = _params.get_value_as_array("props"))
    {
        const auto size = prop_array->size();
        props.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(prop_array->get_at(i)->get_as_collection(), false);
            const auto prop_name = value.get_value_as_string("name");
            const auto prop_value = value.get_value_as_string("value");
            props.emplace_back(prop_name, prop_value);
        }
    }

    g_core->insert_im_stats_event((core::stats::im_stat_event_names)_params.get_value_as_int("event"), std::move(props));
}

void core::im_container::on_speech_to_text(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->speech_to_text(_seq, _params.get_value_as_string("url"), _params.get_value_as_string("locale"));
}

std::shared_ptr<base_im> core::im_container::get_im(coll_helper& _params) const
{
    // temporary, for many im
    return get_default_im();
}

std::shared_ptr<base_im> core::im_container::get_default_im() const
{
    return get_im_by_id(0);
}

std::shared_ptr<base_im> core::im_container::get_im_by_id(int32_t _id) const
{
    if (ims_.empty())
    {
        return std::shared_ptr<base_im>();
    }

    return (*ims_.begin());
}

void core::im_container::on_get_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto force = _params.get_value_as_bool("force", false);
    im->get_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"), force);
}

void core::im_container::on_show_contact_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->show_contact_avatar(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int("size"));
}

void core::im_container::on_remove_contact_avatars(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact_avatars(_seq, _params.get_value_as_string("contact"));
}

void core::im_container::on_delete_archive_messages(int64_t _seq, coll_helper& _params)
{
    im_assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string contact_aimid = _params.get_value_as_string("contact");
    im_assert(!contact_aimid.empty());

    std::vector<archive::delete_message_info> messages;
    core::iarray* array = _params.get_value_as_array("messages");
    const auto size = array->size();
    for (auto i = 0; i < size; ++i)
    {
        auto coll = array->get_at(i)->get_as_collection();
        coll_helper helper(coll, false);
        messages.emplace_back(helper.get_value_as_int64("id"), helper.get_value_as_string("internal_id"), helper.get_value_as_bool("for_all"));
    }

    im->delete_archive_messages(_seq, contact_aimid, messages);
}

void core::im_container::on_delete_archive_messages_from(int64_t _seq, coll_helper& _params)
{
    im_assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string contact_aimid = _params.get_value_as_string("contact");
    im_assert(!contact_aimid.empty());

    const auto from_id = _params.get_value_as_int64("from_id");
    im_assert(from_id >= 0);

    im->delete_archive_messages_from(_seq, contact_aimid, from_id);
}

void core::im_container::on_delete_archive_all_messages(int64_t _seq, coll_helper& _params)
{
    im_assert(_seq > 0);

    auto im = get_im(_params);
    if (!im)
    {
        return;
    }

    const std::string_view contact_aimid = _params.get_value_as_string("contact");
    im_assert(!contact_aimid.empty());

    im->delete_archive_all_messages(_seq, contact_aimid);
}

void core::im_container::on_get_message_context(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_message_context(_seq, _params.get_value_as_string("contact"), _params.get_value_as_int64("msgid"));
}

void core::im_container::on_archive_log_model(int64_t /*_seq*/, coll_helper& /*_params*/)
{
}

void core::im_container::on_add_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_opened_dialog(_params.get_value_as_string("contact"));
}

void core::im_container::on_remove_opened_dialog(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_opened_dialog(_params.get_value_as_string("contact"));
}

void im_container::on_dialogs_search_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    if (!keyword.empty())
    {
        std::vector<std::string> aimids;
        if (_params.is_value_exist("contact"))
            aimids.push_back(_params.get_value_as_string("contact"));

        im->setup_search_dialogs_params(_seq, false);
        im->search_dialogs_local(keyword, aimids);
    }
}

void im_container::on_dialogs_search_local_ended(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->clear_search_dialogs_params(false);
}

void im_container::on_threads_search_local_ended(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->clear_search_dialogs_params(true);
}

void im_container::on_dialogs_search_server(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string contact = _params.get_value_as_string("contact", "");
    if (_params.is_value_exist("cursor"))
    {
        const std::string cursor = _params.get_value_as_string("cursor");
        im->search_dialogs_continue(_seq, cursor, contact);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
        {
            if (!contact.empty())
                im->search_dialogs_one(_seq, keyword, contact);
            else
                im->search_dialogs_all(_seq, keyword);
        }
    }
}

void im_container::on_thread_feed_search_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    if (!keyword.empty())
    {
        const std::string contact = _params.get_value_as_string("contact", "");
        if (!contact.empty())
        {
            im->setup_search_dialogs_params(_seq, false);
            im->search_thread_feed_local(keyword, contact);
        }
    }
}

void im_container::on_threads_search_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    if (!keyword.empty())
    {
        const std::string contact = _params.get_value_as_string("contact", "");
        if (!contact.empty())
        {
            im->setup_search_dialogs_params(_seq, true);
            im->search_threads_local(keyword, contact);
        }
    }
}

void im_container::on_threads_search_server(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string contact = _params.get_value_as_string("contact", "");
    if (_params.is_value_exist("cursor"))
    {
        const std::string cursor = _params.get_value_as_string("cursor");
        im->search_threads_continue(_seq, cursor, contact);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
            im->search_threads(_seq, keyword, contact);
    }
}

void im_container::on_dialogs_search_add_pattern(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string_view contact = _params.get_value_as_string("contact");
    const std::string_view pattern = _params.get_value_as_string("pattern");

    if (!contact.empty() && !pattern.empty())
        im->add_search_pattern_to_history(pattern, contact);
}

void im_container::on_dialogs_search_remove_pattern(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string_view contact = _params.get_value_as_string("contact");
    const std::string_view pattern = _params.get_value_as_string("pattern");
    if (!contact.empty() && !pattern.empty())
        im->remove_search_pattern_from_history(pattern, contact);
}

void im_container::on_set_last_read(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto read_all = _params.get_value_as_bool("read_all", false);

    im->set_last_read(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"), read_all ? core::message_read_mode::read_all : core::message_read_mode::read_text);
}

void im_container::on_set_last_read_mention(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read_mention(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_set_last_read_partial(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_last_read_partial(_params.get_value_as_string("contact"), _params.get_value_as_int64("message"));
}

void im_container::on_hide_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->hide_chat(_params.get_value_as_string("contact"));
}

void im_container::on_mute_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mute_chat(_params.get_value_as_string("contact"), _params.get_value_as_bool("mute"));
}

void im_container::on_upload_file_sharing(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::shared_ptr<core::tools::binary_stream> data = std::make_shared<core::tools::binary_stream>();
    if (_params.is_value_exist("file_stream"))
    {
        auto stream = _params.get_value_as_stream("file_stream");
        auto size = stream->size();
        data->write((char*)(stream->read(size)), size);
    }

    const auto file_name = _params.get_value_as_string("file", "");
    const auto extension = _params.get_value_as_string("ext", "");
    const auto description = _params.get_value_as_string("description", "");
    core::archive::format_data description_format;
    if (_params->is_value_exist("description_format"))
        description_format.unserialize(_params, "description_format");

    core::archive::quotes_vec quotes;
    if (_params->is_value_exist("quotes"))
    {
        core::iarray* array = _params.get_value_as_array("quotes");
        const auto size = array->size();
        quotes.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::archive::quote q;
            q.unserialize(array->get_at(i)->get_as_collection());
            quotes.push_back(std::move(q));
        }
    }

    core::archive::mentions_map mentions;
    if (_params->is_value_exist("mentions"))
    {
        const core::iarray* array = _params.get_value_as_array("mentions");
        for (auto i = 0, size = array->size(); i < size; ++i)
        {
            auto coll = array->get_at(i)->get_as_collection();
            coll_helper helper(coll, false);
            mentions.emplace(helper.get_value_as_string("aimid"), helper.get_value_as_string("friendly"));
        }
    }

    std::optional<int64_t> duration;
    if (_params.is_value_exist("duration"))
        duration = _params.get_value_as_int64("duration");

    auto strip_exif = _params.get_value_as_bool("strip_exif", false);
    auto orientation_tag = _params.get_value_as_int("orientation_tag", 1);//1 is normal orientation

    im->upload_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        file_name,
        std::move(data),
        extension,
        quotes,
        description,
        description_format,
        mentions,
        duration,
        strip_exif,
        orientation_tag);
}

void im_container::on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->abort_file_sharing_upload(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("process_seq"));
}

void im_container::on_get_file_sharing_preview_size(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_file_sharing_preview_size(
        _seq,
        _params.get_value_as_string("url"),
        _params.get_value_as_int("orig_size"));
}

void im_container::on_download_file_sharing_metainfo(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing_metainfo(
        _seq,
        core::tools::filesharing_id(_params.get_value_as_string("file_id"), _params.get_value_as_string("source_id", "")));
}

void im_container::on_download_file(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_file_sharing(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("url"),
        _params.get_value_as_bool("force_request_metainfo"),
        _params.get_value_as_string("filename"),
        _params.get_value_as_string("download_dir"),
        _params.get_value_as_bool("raise", false));
}

void im_container::on_download_image(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_image(
        _seq,
        _params.get_value_as_string("uri"),
        _params.get_value_as_string("destination", ""),
        _params.get_value_as_bool("is_preview"),
        _params.get_value_as_int("preview_width", 0),
        _params.get_value_as_int("preview_height", 0),
        _params.get_value_as_bool("ext_resource", true),
        _params.get_value_as_bool("raise", false));
}

void im_container::on_get_external_file_path(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_external_file_path(_seq, _params.get_value_as_string("url"));
}

void im_container::on_download_link_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->download_link_metainfo(
        _seq,
        _params.get_value_as_string("uri"),
        _params.get_value_as_int("preview_width", 0),
        _params.get_value_as_int("preview_height", 0),
        _params.get_value_as_bool("load_preview", 0),
        _params.get_value_as_string("log_str", ""));
}

static std::optional<int64_t> get_int64_optional(const coll_helper& _params, std::string_view _name)
{
    if (_params.is_value_exist(_name))
        return _params.get_value_as_int64(_name);
    return std::nullopt;
}

void im_container::on_cancel_loader_task(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get_value_as_string("url");

    im->cancel_loader_task(url, get_int64_optional(_params, "seq"));
}

void im_container::on_abort_file_downloading(int64_t /*_seq*/, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto url = _params.get_value_as_string("url");

    im->abort_file_sharing_download(url, get_int64_optional(_params, "seq"));
}

void im_container::on_download_raise_priority(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto proc_id = _params.get_value_as_int64("proc_id");
    im_assert(proc_id > 0);

    im->raise_download_priority(proc_id);
}

void im_container::on_contact_switched(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contact_aimid = _params.get_value_as_string("contact");

    im->contact_switched(contact_aimid);
}

void im_container::on_get_stickers_meta(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_meta(_seq, _params.get_value_as_string("size"));
}

void im_container::on_check_theme_meta_updates(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->check_themes_meta_updates(_seq);
}

void im_container::on_get_theme_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme_wallpaper(_params.get_value_as_string("id"));
}

void im_container::on_get_theme_wallpaper_preview(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_theme_wallpaper_preview(_params.get_value_as_string("id"));
}

void im_container::on_set_user_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::istream* stream = _params.get_value_as_stream("image");
    if (!stream)
    {
        im_assert(!"_params.get_value_as_stream(\"image\")");
        return;
    }

    tools::binary_stream bs_data;
    auto size = stream->size();
    if (stream && size)
    {
        bs_data.write((const char*)stream->read(size), size);
        stream->reset();
    }

    const auto id = _params.get_value_as_string("id", "");
    im->set_user_wallpaper(id, std::move(bs_data));
}

void im_container::on_remove_user_wallpaper(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto id = _params.get_value_as_string("id", "");
    im->remove_user_wallpaper(id);
}

void im_container::on_get_sticker(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto file_id = _params.get_value_as_string("file_id", "");
    auto source_id = _params.get_value_as_string("source_id", "");
    core::tools::filesharing_id filesharing_id(file_id, source_id);

    im->get_sticker(_seq,
        _params.get_value_as_int("set_id", -1),
        _params.get_value_as_int("sticker_id", -1),
        filesharing_id,
        _params.get<core::sticker_size>("size"));
}

void im_container::on_get_sticker_cancel(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto stickers_ids = _params.get_value_as_array("ids");
    if (!stickers_ids)
    {
        im_assert(false);
        return;
    }

    const auto size = stickers_ids->size();
    std::vector<core::tools::filesharing_id> result;
    result.reserve(size);
    for (core::iarray::size_type i = 0; i < size; ++i)
    {
        core::coll_helper value(stickers_ids->get_at(i)->get_as_collection(), false);
        auto file_id = value.get_value_as_string("file_id", "");
        auto source = value.get_value_as_string("source_id", "");
        result.push_back(core::tools::filesharing_id(file_id, source));
    }

    if (result.empty())
        im->get_sticker_cancel(std::move(result), _params.get<core::sticker_size>("size"));
}

void im_container::on_get_fs_stickers_by_ids(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto stikers_ids = _params.get_value_as_array("ids");
    if (!stikers_ids)
    {
        im_assert(false);
        return;
    }
    const auto size = stikers_ids->size();
    std::vector<std::pair<int32_t, int32_t>> result;
    result.reserve(size_t(size) / 2);

    for (core::iarray::size_type i = 0; i < size; i += 2)
        result.emplace_back(stikers_ids->get_at(i)->get_as_int(), stikers_ids->get_at(i + 1)->get_as_int());

    im->get_fs_stickers_by_ids(_seq, std::move(result));
}

void im_container::on_get_stickers_pack_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto file_id = _params.get_value_as_string("file_id", "");
    auto source = _params.get_value_as_string("source", "");

    im->get_stickers_pack_info(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"),
        core::tools::filesharing_id(file_id, source));
}

void im_container::on_add_stickers_pack(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_stickers_pack(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"));
}

void im_container::on_remove_stickers_pack(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_stickers_pack(_seq,
        _params.get_value_as_int("set_id"),
        _params.get_value_as_string("store_id"));
}

void im_container::on_get_stickers_store(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_stickers_store(_seq);
}

void im_container::on_search_stickers_store(int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->search_stickers_store(_seq, _params.get_value_as_string("term"));
}

void im_container::on_get_set_icon_big(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_set_icon_big(_seq, _params.get_value_as_int("set_id"));
}

void im_container::on_clean_set_icon_big(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clean_set_icon_big(_seq, _params.get_value_as_int("set_id"));
}

void im_container::on_set_sticker_order(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<int32_t> values_array;

    core::iarray* vals = _params.get_value_as_array("values");
    if (!vals)
    {
        im_assert(false);
        return;
    }

    values_array.reserve(vals->size());

    for (int32_t i = 0; i < vals->size(); ++i)
    {
        values_array.push_back(vals->get_at(i)->get_as_int());
    }

    im->set_sticker_order(_seq, std::move(values_array));
}

void im_container::on_get_chat_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string aimid, stamp;
    bool threads_auto_subscribe{ false };

    if (_params->is_value_exist("stamp"))
        stamp = _params.get_value_as_string("stamp");

    if (_params->is_value_exist("aimid"))
        aimid = _params.get_value_as_string("aimid");

    if (_params.is_value_exist("threadsAutoSubscribe"))
        threads_auto_subscribe = _params.get_value_as_bool("threadsAutoSubscribe");

    im_assert(!stamp.empty() || !aimid.empty());

    im->get_chat_info(_seq, aimid, stamp, _params.get_value_as_int("limit"), threads_auto_subscribe);
}


void core::im_container::on_thread_autosubscribe(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    bool enable{ false };
    std::string chat_id;

    if (_params.is_value_exist("chat_id"))
        chat_id = _params.get_value_as_string("chat_id");

    if (_params.is_value_exist("thread_auto_subscribe"))
        enable = _params.get_value_as_bool("thread_auto_subscribe");

    im->thread_autosubscribe(_seq, chat_id, enable);
}

void im_container::on_get_chat_home(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto tag = _params.get_value_as_string("tag", "");
    im->get_chat_home(_seq, tag);
}

void im_container::on_resolve_pending(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contacts = to_string_vector(_params, "contacts");
    im->resolve_pending(_seq, _params.get_value_as_string("aimid"), contacts, _params.get_value_as_bool("approve"));
}

void core::im_container::on_get_chat_member_info(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (const auto members = to_string_vector(_params, "members"); !members.empty())
        im->get_chat_member_info(_seq, _params.get_value_as_string("aimid"), members);
}

void im_container::on_get_chat_members(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
        im->get_chat_members_next_page(_seq, aimid, _params.get_value_as_string("cursor"), page_size);
    else
        im->get_chat_members_page(_seq, aimid, _params.get_value_as_string("role"), page_size);
}

void core::im_container::on_search_chat_members(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
    {
        im->search_chat_members_next_page(_seq, aimid, _params.get_value_as_string("cursor"), page_size);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
            im->search_chat_members_page(_seq, aimid, _params.get_value_as_string("role", ""), keyword, page_size);
    }
}

void im_container::on_get_chat_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_chat_contacts(_seq,
        _params.get_value_as_string("aimid"),
        _params.get_value_as_string("cursor", ""),
        _params.get_value_as_uint("page_size"));
}

void im_container::on_search_contacts_server(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const std::string keyword = _params.get_value_as_string("keyword");
    const std::string phone = _params.get_value_as_string("phoneNumber", "");

    if (!keyword.empty() || !phone.empty())
        im->search_contacts_server(_seq, keyword, phone);
}

void core::im_container::on_search_contacts_local(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    g_core->begin_cl_search();

    std::vector<std::vector<std::string>> symbols_patterns;
    std::string pattern;
    uint32_t fix_pat_count = 0;
    if (_params.is_value_exist("symbols_array"))
    {
        const auto searchSymbolsArray = _params.get_value_as_array("symbols_array");
        const auto search_symbol_size = searchSymbolsArray->size();
        symbols_patterns.reserve(search_symbol_size);
        for (auto i = 0; i < search_symbol_size; ++i)
        {
            core::coll_helper symbol_value(searchSymbolsArray->get_at(i)->get_as_collection(), false);
            symbols_patterns.push_back(to_string_vector(symbol_value, "symbols_patterns"));
        }

        fix_pat_count = _params.get_value_as_uint("fixed_patterns_count", 0);
    }
    else if (_params->is_value_exist("init_pattern"))
    {
        pattern = _params.get_value_as_string("init_pattern");
    }

    for (auto& symbol_iter : symbols_patterns)
    {
        for (auto& iter : symbol_iter)
            iter = ::core::tools::system::to_upper(iter);
    }

    im->search_contacts_local(symbols_patterns, _seq, fix_pat_count, pattern);
}

void core::im_container::on_syncronize_addressbook(int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto phones = to_string_vector(_params, "phones");
    im->syncronize_address_book(_seq, _params.get_value_as_string("name"), phones);
}

void im_container::on_hide_dlg_state(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto contacts = to_string_vector(_params, "contacts");
    for (const auto& c : contacts)
        im->hide_dlg_state(c);
}

void im_container::on_set_attention_attribute(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->set_attention_attribute(_seq, _params.get_value_as_string("contact"), _params.get_value_as_bool("value"), false);
}

void im_container::on_add_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->add_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("group"),
        _params.get_value_as_string("message"));
}

void im_container::on_remove_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_contact(
        _seq,
        _params.get_value_as_string("contact"));
}

void im_container::on_rename_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->rename_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("friendly")
    );
}

void im_container::on_ignore_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->ignore_contact(
        _seq,
        _params.get_value_as_string("contact"), _params.get_value_as_bool("ignore"));
}

void im_container::on_get_ignore_contacts(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ignore_list(_seq);
}

void im_container::on_pin_chat(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->pin_chat(_params.get_value_as_string("contact"));
}

void im_container::on_unfavorite(int64_t _seq, core::coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->unpin_chat(_params.get_value_as_string("contact"));
}

void im_container::on_mark_unimportant(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mark_unimportant(_params.get_value_as_string("contact"));
}

void im_container::on_remove_from_unimportant(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_from_unimportant(_params.get_value_as_string("contact"));
}

void core::im_container::on_update_profile(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::vector<std::pair<std::string, std::string>> fields;

    if (const auto field_array = _params.get_value_as_array("fields"))
    {
        const auto size = field_array->size();
        fields.reserve(size);
        for (auto i = 0; i < size; ++i)
        {
            core::coll_helper value(field_array->get_at(i)->get_as_collection(), false);
            auto field_name = value.get_value_as_string("field_name");
            auto field_value = value.get_value_as_string("field_value");
            fields.push_back(std::make_pair(field_name, field_value));
        }
    }

    im->update_profile(_seq, fields);
}

void core::im_container::on_join_livechat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string stamp = _params.get_value_as_string("stamp");

    im->join_live_chat(_seq, stamp);
}

void im_container::on_cancel_join_livechat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->cancel_pending_join(_params.get_value_as_string("sn"));
}

void im_container::on_cancel_chat_invitation(int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->cancel_pending_invitation(_params.get_value_as_string("contact"), _params.get_value_as_string("chat"));
}

void core::im_container::on_set_locale(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    std::string locale = _params.get_value_as_string("locale");

    g_core->set_locale(locale);
}

void core::im_container::on_set_user_proxy(int64_t _seq, coll_helper& _params)
{
    proxy_settings user_proxy;

    user_proxy.proxy_type_ = (int32_t)_params.get_value_as_enum<proxy_type>("settings_proxy_type");

    im_assert(user_proxy.proxy_type_ >= 0 || user_proxy.proxy_type_ == -1);

    user_proxy.use_proxy_ = user_proxy.proxy_type_ != -1;
    user_proxy.proxy_server_ = _params.get_value_as_string("settings_proxy_server");
    user_proxy.proxy_port_ = _params.get_value_as_int("settings_proxy_port");
    user_proxy.login_ = _params.get_value_as_string("settings_proxy_username");
    user_proxy.password_ = _params.get_value_as_string("settings_proxy_password");
    user_proxy.need_auth_ = _params.get_value_as_bool("settings_proxy_need_auth");
    user_proxy.auth_type_ = _params.get_value_as_enum<core::proxy_auth>("settings_proxy_auth_type");

    g_core->set_user_proxy_settings(user_proxy);
}

void core::im_container::on_set_avatar(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    tools::binary_stream bs_data;

    core::istream* stream = _params.get_value_as_stream("avatar");
    if (!stream)
    {
        im_assert(!"_params.get_value_as_stream(\"avatar\")");
        return;
    }

    auto size = stream->size();
    if (stream && size)
    {
        bs_data.write((const char*)stream->read(size), size);
        stream->reset();
    }

    im->set_avatar(_seq, std::move(bs_data), _params.get_value_as_string("aimid", ""), _params.get_value_as_bool("chat", false));
}

void core::im_container::on_create_chat(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto params = core::wim::chat_params::create(_params);
    auto members = to_string_vector(_params, "members");
    im->create_chat(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"), std::move(members), std::move(params));
}

void core::im_container::on_mod_chat_params(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    auto params = core::wim::chat_params::create(_params);
    im->mod_chat_params(_seq, _params.get_value_as_string("aimid"), std::move(params));
}

void core::im_container::on_mod_chat_name(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_name(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("name"));
}

void core::im_container::on_mod_chat_about(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_about(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("about"));
}

void core::im_container::on_mod_chat_rules(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->mod_chat_rules(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("rules"));
}

void core::im_container::on_chat_pin_message(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->pin_chat_message(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_int64("msgId"), _params.get_value_as_bool("unpin"));
}

void core::im_container::on_block_chat_member(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->block_chat_member(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_bool("block"), _params.get_value_as_bool("remove_messages"));
}

void core::im_container::on_set_chat_member_role(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->set_chat_member_role(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("contact"), _params.get_value_as_string("role"));
}

bool im_container::has_valid_login() const
{
    auto im = get_im_by_id(0);
    if (!im)
        return false;

    return im->has_valid_login();
}

std::string core::im_container::get_login() const
{
    auto im = get_im_by_id(0);
    if (!im)
        return std::string();

    return im->get_login();
}

void im_container::on_get_mentions_suggests(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_mentions_suggests(_seq, _params.get_value_as_string("aimid"), _params.get_value_as_string("pattern"));
}

void im_container::on_get_code_by_phone_call(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_code_by_phone_call(_params.get_value_as_string("ivr_url"));
}

void im_container::on_get_logs_path(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_logs_path(_seq);
}

void im_container::on_change_app_config(const int64_t _seq, coll_helper& _params)
{
    // TODO: normal serialize / deserialize into a struct

    coll_helper cl_coll(g_core->create_collection(), true);

    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::full_log, _params.get_value_as_bool("fulllog"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::is_updateble, _params.get_value_as_bool("updateble"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::unlock_context_menu_features, _params.get_value_as_bool("dev.unlock_context_menu_features"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::show_msg_ids, _params.get_value_as_bool("dev.show_message_ids"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::save_rtp_dumps, _params.get_value_as_bool("dev.save_rtp_dumps"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::cache_history_pages_secs, _params.get_value_as_int("dev.cache_history_pages_secs"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::cache_history_pages_check_interval_secs, _params.get_value_as_int("dev.cache_history_pages_check_interval_secs"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::is_server_search_enabled, _params.get_value_as_bool("dev.server_search"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::dev_id, std::string(_params.get_value_as_string("dev_id")));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::watch_gui_memory, _params.get_value_as_bool("dev.watch_gui_memory"));
    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::net_compression, _params.get_value_as_bool("dev.net_compression"));

    const auto app_ini_path = utils::get_app_ini_path();

    configuration::dump_app_config_to_disk(app_ini_path);

    cl_coll.set_value_as_int("error", 0);

    g_core->post_message_to_gui("app_config_changed", _seq, cl_coll.get());

}

void im_container::on_remove_content_cache(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_content_cache(_seq);
}

void im_container::on_clear_avatars(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->clear_avatars(_seq);
}

void core::im_container::on_remove_omicron_stg(const int64_t _seq, coll_helper & _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->remove_omicron_stg(_seq);
}

void im_container::on_report_contact(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->report_contact(
        _seq,
        _params.get_value_as_string("contact"),
        _params.get_value_as_string("reason"),
        _params.get_value_as_bool("ignoreAndClose", true));
}

void im_container::on_report_stickerpack(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_stickerpack(_params.get_value_as_int("id"), _params.get_value_as_string("reason"));
}

void im_container::on_report_sticker(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_sticker(_params.get_value_as_string("id"), _params.get_value_as_string("reason"), _params.get_value_as_string("contact"), _params.get_value_as_string("chatId"));
}

void im_container::on_report_message(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->report_message(_params.get_value_as_int64("id"), _params.get_value_as_string("text"), _params.get_value_as_string("reason"), _params.get_value_as_string("contact"), _params.get_value_as_string("chatId"));
}

void im_container::on_request_memory_usage(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->request_memory_usage(_seq);
}

void im_container::on_report_memory_usage(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    core::memory_stats::partial_response part_response;
    if (!part_response.unserialize(_params))
        return;

    im->report_memory_usage(_seq,
                            _params.get_value_as_int64("request_id"),
                            part_response);
}

void im_container::on_get_ram_usage(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->get_ram_usage(_seq);
}

void im_container::on_make_archive_holes(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->make_archive_holes(_seq, _params.get_value_as_string("archive"));
}

void im_container::on_invalidate_history(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    im->invalidate_archive_history(_seq, _params.get_value_as_string("archive"));
}

void im_container::on_invalidate_archive_data(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params); im)
    {
        if (_params.is_value_exist("ids"))
        {
            if (auto ids_array = _params.get_value_as_array("ids"))
            {
                std::vector<int64_t> ids;
                const auto size = ids_array->size();
                ids.reserve(size);
                for (auto i = 0; i < size; ++i)
                    ids.emplace_back(ids_array->get_at(i)->get_as_int64());
                im->invalidate_archive_data(_seq, _params.get_value_as_string("aimid"), std::move(ids));
            }
        }
        else
        {
            const int64_t from = _params.get_value_as_int64("from", 0);
            const int64_t before = _params.get_value_as_int64("before", 0);
            const int64_t after = _params.get_value_as_int64("after", 0);
            im->invalidate_archive_data(_seq, _params.get_value_as_string("aimid"), from, before, after);
        }
    }
}

void im_container::on_user_accept_gdpr(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (im)
        im->user_accept_gdpr(_seq);
}

void im_container::set_user_has_been_logged_in_ever(bool _hasEverLoggedIn)
{
    if (core::configuration::get_app_config().gdpr_user_has_logged_in_ever() == _hasEverLoggedIn)
        return;

    core::configuration::set_config_option(core::configuration::app_config::AppConfigOption::gdpr_user_has_logged_in_ever,
        std::move(_hasEverLoggedIn));

    const auto app_ini_path = utils::get_app_ini_path();
    configuration::dump_app_config_to_disk(app_ini_path);

    // Report new config to GUI
    core::coll_helper cl_coll(g_core->create_collection(), true);
    configuration::get_app_config().serialize(Out cl_coll);
    g_core->post_message_to_gui("app_config", 0, cl_coll.get());
}

void im_container::on_get_dialog_gallery(const int64_t _seq, coll_helper &_params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto page_size = _params.get_value_as_int("page_size");
    archive::gallery_entry_id after;
    if (_params.is_value_exist("after_msg"))
        after = archive::gallery_entry_id(_params.get_value_as_int64("after_msg"), _params.get_value_as_int64("after_seq"));

    const auto type = to_string_vector(_params, "type");

    bool download_holes = _params.get_value_as_bool("download_holes");

    auto im = get_im(_params);
    if (im)
        im->get_dialog_gallery(_seq, aimid, type, after, page_size, download_holes);
}

void im_container::on_get_dialog_gallery_by_msg(const int64_t _seq, coll_helper &_params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    int64_t msg_id = _params.get_value_as_int64("msg");

    const auto type = to_string_vector(_params, "type");

    auto im = get_im(_params);
    if (im)
        im->get_dialog_gallery_by_msg(_seq, aimid, type, msg_id);
}

void im_container::on_request_gallery_state(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");

    auto im = get_im(_params);
    if (im)
        im->request_gallery_state(aimid);
}

void im_container::on_get_gallery_index(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto msg = _params.get_value_as_int64("msg");
    const auto seq = _params.get_value_as_int64("seq");
    const auto type = to_string_vector(_params, "type");

    auto im = get_im(_params);
    if (im)
        im->get_gallery_index(aimid, type, msg, seq);
}

void im_container::on_make_gallery_hole(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    const auto from = _params.get_value_as_int64("from");
    const auto till = _params.get_value_as_int64("till");

    auto im = get_im(_params);
    if (im)
        im->make_gallery_hole(aimid, from, till);
}

void im_container::on_stop_gallery_holes_downloading(const int64_t _seq, coll_helper& _params)
{
    const auto aimid = _params.get_value_as_string("aimid");
    auto im = get_im(_params);
    if (im)
        im->stop_gallery_holes_downloading(aimid);
}

void im_container::on_ui_activity(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_ui_activity(_params.get_value_as_int64("time"));
}

void im_container::on_close_stranger(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->close_stranger(_params.get_value_as_string("contact"));
}

void im_container::on_local_pin_set(const int64_t _seq, coll_helper& _params)
{
    const auto password = _params.get_value_as_string("password");
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_set(password);
}

void im_container::on_local_pin_entered(const int64_t _seq, coll_helper& _params)
{
    const auto password = _params.get_value_as_string("password");
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_entered(_seq, password);
}

void im_container::on_local_pin_disable(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_local_pin_disable(_seq);
}

void im_container::on_update_check(const int64_t _seq, coll_helper &_params)
{
    auto url = (_params.get() && _params.is_value_exist("url")) ? std::make_optional<std::string>(_params.get_value_as_string("url")) : std::nullopt;
    g_core->check_update(_seq, std::move(url));
}

void im_container::on_get_id_info(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_id_info(_seq, _params.get_value_as_string("id"));
}

void im_container::on_get_user_info(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_user_info(_seq, _params.get_value_as_string("contact"));
}

void im_container::on_set_privacy_settings(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
    {
        wim::privacy_settings settings;
        settings.set_access_value(_params.get_value_as_string("group"), _params.get_value_as_enum<privacy_access_right>("value"));

        im->set_privacy_settings(_seq, std::move(settings));
    }
}

void im_container::on_get_privacy_settings(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_privacy_settings(_seq);
}

void im_container::on_get_user_last_seen(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_user_last_seen(_seq, to_string_vector(_params, "contacts"));
}

void im_container::on_nickname_check(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (im)
        im->on_nickname_check(_seq, _params.get_value_as_string("nick"), _params.get_value_as_bool("set_nick"));
}

void im_container::on_group_nickname_check(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (im)
        im->on_group_nickname_check(_seq, _params.get_value_as_string("nick"));
}

void im_container::on_get_common_chats(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->on_get_common_chats(_seq, _params.get_value_as_string("contact"));
}

void im_container::on_reset_connection(const int64_t _seq, coll_helper& _params)
{
    g_core->reset_connection();
}

void im_container::on_get_sticker_suggests(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_sticker_suggests(_params.get_value_as_string("contact"), _params.get_value_as_string("text"));
}

void im_container::on_clear_smartreply_suggests(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    if (_params.is_value_exist("msgId"))
        im->clear_smartreply_suggests_for_message(_params.get_value_as_string("contact"), _params.get_value_as_int64("msgId"));
    else
        im->clear_smartreply_suggests(_params.get_value_as_string("contact"));
}

void im_container::on_load_smartreply_suggests(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->load_cached_smartreplies(_params.get_value_as_string("contact"));
}

void im_container::on_get_smartreplies(int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
    {
        std::vector<smartreply::type> types;
        if (auto arr = _params.get_value_as_array("types"))
        {
            const auto size = arr->size();
            types.reserve(size);
            for (int i = 0; i < size; ++i)
                types.emplace_back((smartreply::type)arr->get_at(i)->get_as_int());
        }
        im->get_smartreplies(_params.get_value_as_string("contact"), _params.get_value_as_int64("msgid"), std::move(types));
    }
}

void im_container::on_update_recent_avatars_size(const int64_t _seq, coll_helper& _params)
{
    if (_params.is_value_exist("size"))
        g_core->set_recent_avatars_size(_params.get_value_as_int("size"));
}

void im_container::on_set_install_beta_updates(const int64_t _seq, coll_helper& _params)
{
    if (_params.is_value_exist("enabled"))
        g_core->set_install_beta_updates(_params.get_value_as_bool("enabled"));
}

void im_container::on_get_poll(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_poll(_seq, _params.get_value_as_string("id"));
}

void im_container::on_vote_in_poll(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->vote_in_poll(_seq, _params.get_value_as_string("id"), _params.get_value_as_string("answer_id"));
}

void im_container::on_revoke_vote(const int64_t _seq, coll_helper &_params)
{
    auto im = get_im(_params);
    if (im)
        im->revoke_vote(_seq, _params.get_value_as_string("id"));
}

void im_container::on_stop_poll(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->stop_poll(_seq, _params.get_value_as_string("id"));
}

void core::im_container::on_edit_task(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        core::tasks::task_change task;
        if (task.unserialize(_params.get_value_as_collection("task")))
            im->edit_task(_seq, task);
    }
}

void core::im_container::on_create_task(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        core::tasks::task_data task;
        if (task.unserialize(_params.get_value_as_collection("task")))
            im->create_task(_seq, task);
    }
}

void core::im_container::on_request_initial_tasks(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->request_initial_tasks(_seq);
}

void core::im_container::on_task_update_last_used(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->update_task_last_used(_seq, _params.get_value_as_string("task_id"));
}

void im_container::on_group_subscribe(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        std::string aimid, stamp;

        if (_params->is_value_exist("stamp"))
            stamp = _params.get_value_as_string("stamp");

        if (_params->is_value_exist("aimid"))
            aimid = _params.get_value_as_string("aimid");

        im_assert(!stamp.empty() || !aimid.empty());

        im->group_subscribe(_seq, stamp, aimid);
    }
}

void im_container::on_group_cancel_subscription(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        std::string aimid, stamp;

        if (_params->is_value_exist("stamp"))
            stamp = _params.get_value_as_string("stamp");

        if (_params->is_value_exist("aimid"))
            aimid = _params.get_value_as_string("aimid");

        im_assert(!stamp.empty() || !aimid.empty());

        im->cancel_group_subscription(_seq, stamp, aimid);
    }
}

void im_container::on_group_inviteblacklist_add(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->add_to_inviters_blacklist(to_string_vector(_params, "contacts"));
}

void im_container::on_group_inviteblacklist_remove(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->remove_from_inviters_blacklist(to_string_vector(_params, "contacts"), _params.get_value_as_bool("all"));
}

void im_container::on_group_inviteblacklist_search(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        im->search_inviters_blacklist(
            _seq,
            _params.get_value_as_string("keyword"),
            _params.get_value_as_string("cursor"),
            _params.get_value_as_uint("page_size"));
    }
}

void im_container::group_blacklisted_cl_inviters(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_blacklisted_cl_inviters(_params.get_value_as_string("cursor"), _params.get_value_as_uint("page_size"));
}

void im_container::on_suggest_group_nick(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->suggest_group_nick(_seq, _params.get_value_as_string("sn"), _params.get_value_as_bool("public"));
}

void im_container::on_get_bot_callback_answer(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->get_bot_callback_answer(_seq, _params.get_value_as_string("chat_id"), _params.get_value_as_string("callback_data"), _params.get_value_as_int64("msg_id"));
}

void im_container::on_start_bot(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (im)
        im->start_bot(_seq, _params.get_value_as_string("nick"), _params.get_value_as_string("params"));
}

void im_container::on_create_confrence(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        im->create_conference(
            _seq,
            _params.get_value_as_string("name"),
            _params.get_value_as_bool("is_webinar"),
            to_string_vector(_params, "participants"),
            _params.get_value_as_bool("call_participants")
            );
    }
}

void im_container::on_get_sessions(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_sessions();
}

void im_container::on_reset_session(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        std::string_view hash = _params.is_value_exist("hash") ? _params.get_value_as_string("hash") : std::string_view();
        im->reset_session(hash);
    }
}

void im_container::on_status_set(const int64_t _seq, coll_helper &_params)
{
    if (auto im = get_im(_params))
        im->set_status(_params.get_value_as_string("status"), std::chrono::seconds(_params.get_value_as_int64("duration")), _params.get_value_as_string("description", ""));
}

void im_container::on_subscribe_status(const int64_t _seq, coll_helper &_params)
{
    if (auto im = get_im(_params))
        im->subscribe_status(to_string_vector(_params, "contacts"));
}

void im_container::on_unsubscribe_status(const int64_t _seq, coll_helper &_params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_status(to_string_vector(_params, "contacts"));
}

void im_container::on_subscribe_call_room_info(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_call_room_info(_params.get_value_as_string("room_id"));
}

void im_container::on_unsubscribe_call_room_info(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_call_room_info(_params.get_value_as_string("room_id"));
}

void im_container::on_subscribe_thread_events(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_thread_updates(to_string_vector(_params, "contacts"));
}

void im_container::on_unsubscribe_thread_events(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_thread_updates(to_string_vector(_params, "contacts"));
}

void im_container::on_subscribe_task(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_task(_params.get_value_as_string("task_id"));
}

void im_container::on_unsubscribe_task(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_task(_params.get_value_as_string("task_id"));
}

void im_container::on_subscribe_filesharing_antivirus(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        core::tools::filesharing_id filesharing_id(_params.get_value_as_string("file_id", ""), _params.get_value_as_string("source_id", ""));
        im->subscribe_filesharing_antivirus(filesharing_id);
    }
}

void im_container::on_unsubscribe_filesharing_antivirus(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        core::tools::filesharing_id filesharing_id(_params.get_value_as_string("file_id", ""), _params.get_value_as_string("source_id", ""));
        im->unsubscribe_filesharing_antivirus(filesharing_id);
    }
}

void core::im_container::on_subscribe_mails_counter(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_mails_counter();
}

void core::im_container::on_unsubscribe_mails_counter(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_mails_counter();
}

void im_container::on_subscribe_tasks_counter(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_tasks_counter();
}

void im_container::on_unsubscribe_tasks_counter(const int64_t /*_seq*/, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_tasks_counter();
}

void im_container::on_get_emoji(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_emoji(_seq, _params.get_value_as_string("code"), _params.get_value_as_int("size"));
}

void im_container::on_send_notify_sms(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->send_notify_sms(_params.get_value_as_string("smsNotifyContext"));
}

void im_container::on_add_thread(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        archive::thread_parent_topic topic;
        topic.unserialize(_params);
        im->add_thread(_seq, std::move(topic));
    }
}

void im_container::on_get_thread(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_thread_info(_seq, _params.get_value_as_string("thread_id"));
}

void im_container::on_threads_feed_get(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_threads_feed(_seq, _params.get_value_as_string("cursor"));
}

void im_container::on_subscribe_thread(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->subscribe_thread(_seq, _params.get_value_as_string("thread_id"));
}

void im_container::on_unsubscribe_thread(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->unsubscribe_thread(_seq, _params.get_value_as_string("thread_id"));
}

void im_container::on_threads_subscribers_get(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto thread_id = _params.get_value_as_string("thread_id");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
        im->get_thread_subscribers_next_page(_seq, thread_id, _params.get_value_as_string("cursor"), page_size);
    else
        im->get_thread_subscribers_page(_seq, thread_id, _params.get_value_as_string("role"), page_size);
}

void im_container::on_threads_subscribers_search(const int64_t _seq, coll_helper& _params)
{
    auto im = get_im(_params);
    if (!im)
        return;

    const auto thread_id = _params.get_value_as_string("thread_id");
    const auto page_size = _params.get_value_as_uint("page_size");

    if (_params.is_value_exist("cursor"))
    {
        im->search_thread_subscribers_next_page(_seq, thread_id, _params.get_value_as_string("cursor"), page_size);
    }
    else
    {
        const std::string keyword = _params.get_value_as_string("keyword");
        if (!keyword.empty())
            im->search_thread_subscribers_page(_seq, thread_id, _params.get_value_as_string("role"), keyword, page_size);
    }
}

void im_container::on_chats_thread_add(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->add_chat_thread(_params.get_value_as_string("chat_id"), _params.get_value_as_int64("msg_id"), _params.get_value_as_string("thread_id"));
}

void im_container::on_chats_thread_remove(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        if (const std::string chat_id = _params.get_value_as_string("chat_id"); !chat_id.empty())
        {
            if (_params.is_value_exist("msg_id"))
                im->remove_chat_thread(chat_id, _params.get_value_as_int64("msg_id"));
            else
                im->remove_chat_thread(chat_id, _params.get_value_as_string("thread_id"));
        }
    }
}

void im_container::on_draft_set(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->set_draft(_params.get_value_as_string("contact"), make_pack(_params), _params.get_value_as_int64("timestamp"), _params.get_value_as_bool("sync"));
}

void im_container::on_draft_get(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->get_draft(_params.get_value_as_string("contact"));
}

void im_container::on_get_reactions(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        core::archive::msgids_list ids;
        if (auto idsArray = _params.get_value_as_array("ids"))
        {
            const auto size = idsArray->size();
            ids.reserve(size);
            for (std::remove_const_t<decltype(size)> i = 0; i < size; ++i)
                ids.push_back(idsArray->get_at(i)->get_as_int64());
        }

        im->get_reactions(_params.get_value_as_string("chat_id"), std::make_shared<core::archive::msgids_list>(std::move(ids)), _params.get_value_as_bool("first_load"));
    }
}

void im_container::on_add_reaction(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {

        std::vector<std::string> reactions;
        if (auto array = _params.get_value_as_array("reactions"))
        {
            const auto size = array->size();
            reactions.reserve(size);
            for (int i = 0; i < size; ++i)
                reactions.emplace_back(array->get_at(i)->get_as_string());
        }

        im->add_reaction(_seq, _params.get_value_as_int64("msg_id"), _params.get_value_as_string("chat_id"), _params.get_value_as_string("reaction"), reactions);
    }
}

void im_container::on_remove_reaction(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->remove_reaction(_seq, _params.get_value_as_int64("msg_id"), _params.get_value_as_string("chat_id"));
}

void im_container::on_list_reactions(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
    {
        im->list_reactions(_seq,
                           _params.get_value_as_int64("msg_id"),
                           _params.get_value_as_string("chat_id"),
                           _params.get_value_as_string("reaction"),
                           _params.get_value_as_string("newer_than"),
                           _params.get_value_as_string("older_than"),
                           _params.get_value_as_int64("limit"));
    }
}

void im_container::on_miniapp_start_session(const int64_t _seq, coll_helper& _params)
{
    if (auto im = get_im(_params))
        im->start_miniapp_session(_params.get_value_as_string("id"));
}
