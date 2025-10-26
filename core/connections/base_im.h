#pragma once

#include "../../common.shared/patch_version.h"

#include "wim/privacy_settings.h"
#include "archive/history_message.h"
#include "smartreply/smartreply_marker.h"
#ifndef STRIP_VOIP
#include "../Voip/VoipManager.h"
#endif

namespace voip_manager {
    class VoipManager;
    struct VoipProtoMsg;
    struct WindowParams;
    struct VoipProxySettings;
    enum AvatarThemeType : unsigned short;
}

namespace core
{
    class login_info;
    class im_login_id;
    class phone_info;
    class search_params;
    enum class file_sharing_function;
    enum class message_type;
    enum class sticker_size;
    enum class profile_state;
    enum class typing_status;
    enum class message_read_mode;
    class auto_callback;

    namespace wim
    {
        class im;
        class wim_packet;
        struct wim_packet_params;
        class chat_params;
        struct auth_parameters;
        enum class is_ping;
        enum class is_login;
        enum class is_updated_messages;
    }

    namespace archive
    {
        class message_header;
        class quote;
        struct shared_contact_data;
        struct geo_data;
        struct thread_parent_topic;

        struct gallery_entry_id;
        typedef std::vector<int64_t> msgids_list;
        typedef std::vector<quote> quotes_vec;
        typedef std::map<std::string, std::string> mentions_map;
        using shared_contact = std::optional<shared_contact_data>;
        using geo = std::optional<geo_data>;
        using poll = std::optional<poll_data>;

        struct message_pack
        {
            std::string message_;
            core::data::format message_format_;
            quotes_vec quotes_;
            mentions_map mentions_;
            std::string internal_id_;
            core::message_type type_;
            uint64_t message_time_ = 0;
            std::string description_;
            core::data::format description_format_;
            std::string url_;
            common::tools::patch_version version_;
            shared_contact shared_contact_;
            geo geo_;
            poll poll_;
            core::tasks::task task_;
            smartreply::marker_opt smartreply_marker_;
            bool channel_ = false;
            std::string chat_sender_;
            std::optional<int64_t> draft_delete_time_;
        };

        struct delete_message_info
        {
            delete_message_info(int64_t _id, const std::string& _internal_id, bool _for_all)
                : id_(_id)
                , internal_id_(_internal_id)
                , for_all_(_for_all)
            {
            }

            int64_t id_;
            std::string internal_id_;
            bool for_all_;
        };
    }

    namespace stickers
    {
        class face;
    }

    namespace smartreply
    {
        enum class type;
    }

    namespace themes
    {
        class wallpaper_loader;
    }

    namespace  memory_stats
    {
        class memory_stats_collector;
        struct request_handle;
        struct partial_response;
        using request_id = int64_t;
    }

    namespace tools
    {
        class filesharing_id;
    }

    class base_im
    {
        std::shared_ptr<voip_manager::VoipManager> voip_manager_;
        int32_t id_;

        // stickers
        // use it only this from thread
        std::shared_ptr<stickers::face> stickers_;

        std::shared_ptr<themes::wallpaper_loader> wp_loader_;

    protected:
        std::shared_ptr<core::memory_stats::memory_stats_collector> memory_stats_collector_;

        std::wstring get_contactlist_file_name() const;
        std::wstring get_my_info_file_name() const;
        std::wstring get_active_dialogs_file_name() const;
        std::wstring get_pinned_chats_file_name() const;
        std::wstring get_unimportant_file_name() const;
        std::wstring get_mailboxes_file_name() const;
        std::wstring get_im_data_path() const;
        std::wstring get_avatars_data_path() const;
        std::wstring get_file_name_by_url(const std::string& _url) const;
        std::wstring get_stickers_path() const;
        std::wstring get_im_downloads_path(const std::string &alt) const;
        std::wstring get_content_cache_path() const;
        std::wstring get_last_suggest_file_name() const;
        std::wstring get_search_history_path() const;
        std::wstring get_call_log_file_name() const;
        std::wstring get_inviters_blacklist_file_name() const;
        std::wstring get_tasks_file_name() const;

        virtual std::string _get_protocol_uid() = 0;
        void set_id(int32_t _id);
        int32_t get_id() const;

        std::shared_ptr<stickers::face> get_stickers();
        std::shared_ptr<themes::wallpaper_loader> get_wallpaper_loader();

        std::vector<std::string> get_audio_devices_names();
        std::vector<std::string> get_video_devices_names();

    public:
        base_im(const im_login_id& _login,
                std::shared_ptr<voip_manager::VoipManager> _voip_manager,
                std::shared_ptr<memory_stats::memory_stats_collector> _memory_stats_collector);
        virtual ~base_im();

        virtual void connect() = 0;
        virtual std::wstring get_im_path() const = 0;

        // login functions
        virtual void login(int64_t _seq, const login_info& _info) = 0;
        virtual void login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login) = 0;
        virtual void login_get_sms_code(int64_t _seq, const phone_info& _info, bool _is_login) = 0;
        virtual void login_by_phone(int64_t _seq, const phone_info& _info) = 0;
        virtual void login_by_oauth2(int64_t _seq, std::string_view _authcode) = 0;

        virtual void start_attach_phone(int64_t _seq, const phone_info& _info) = 0;

        virtual std::string get_login() const = 0;
        virtual void logout(std::function<void()> _on_result) = 0;

        virtual wim::wim_packet_params make_wim_params() const = 0;
        virtual wim::wim_packet_params make_wim_params_general(bool _is_current_auth_params) const = 0;

        virtual void erase_auth_data() = 0; // when logout
        virtual void start_session(wim::is_ping _is_ping, wim::is_login _is_login) = 0;
        virtual void handle_net_error(const std::string& _url, int32_t err) = 0;

        virtual void phoneinfo(int64_t _seq, const std::string &phone, const std::string &gui_locale) = 0;

        // messaging functions
        virtual void send_message_to_contact(
            const int64_t _seq,
            const std::string& _contacts,
            core::archive::message_pack _pack) = 0;

        virtual void send_message_to_contacts(
            const int64_t _seq,
            const std::vector<std::string>& _contact,
            core::archive::message_pack _pack) = 0;

        virtual void update_message_to_contact(
            const int64_t _seq,
            const int64_t _id,
            const std::string& _contact,
            core::archive::message_pack _pack) = 0;

        virtual void set_draft(const std::string& _contact, core::archive::message_pack _pack, int64_t _timestamp, bool _sync) = 0;
        virtual void get_draft(const std::string& _contact) = 0;

        virtual void send_message_typing(const int64_t _seq, const std::string& _contact, const core::typing_status& _status, const std::string& _id) = 0;

        // state
        virtual void set_state(const int64_t, const core::profile_state) = 0;

        // group chat
        virtual void remove_members(std::string _aimid, std::vector<std::string> _members_to_delete) = 0;
        virtual void add_members(std::string _aimid, std::vector<std::string> _m_chat_members_to_add, bool _unblock) = 0;

        // mrim
        virtual void get_mrim_key(int64_t _seq, const std::string& _email) = 0;

        // avatar function
        virtual void get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size, bool _force) = 0;
        virtual void show_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size) = 0;
        virtual void remove_contact_avatars(int64_t _seq, const std::string& _contact) = 0;

        // history functions
        virtual void get_archive_messages(int64_t _seq_, const std::string& _contact, int64_t _from, int64_t _count_early, int64_t _count_later, bool _need_prefetch, bool _first_request, bool _after_search) = 0;
        virtual void get_archive_messages_buddies(int64_t _seq_, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids, wim::is_updated_messages) = 0;
        virtual void set_last_read(const std::string& _contact, int64_t _message, message_read_mode _mode) = 0;
        virtual void set_last_read_mention(const std::string& _contact, int64_t _message) = 0;
        virtual void set_last_read_partial(const std::string& _contact, int64_t _message) = 0;
        virtual void hide_dlg_state(const std::string& _contact) = 0;
        virtual void delete_archive_messages(const int64_t _seq, const std::string &_contact_aimid, const std::vector<archive::delete_message_info>& _messages) = 0;
        virtual void delete_archive_messages_from(const int64_t _seq, const std::string &_contact_aimid, const int64_t _from_id) = 0;
        virtual void delete_archive_all_messages(const int64_t _seq, std::string_view _contact_aimid) = 0;
        virtual void get_messages_for_update(const std::string& _contact) = 0;

        virtual void get_mentions(int64_t _seq, std::string_view _contact) = 0;

        virtual void get_message_context(int64_t _seq, const std::string& _contact, const int64_t _msgid) = 0;

        virtual void add_opened_dialog(const std::string& _contact) = 0;
        virtual void remove_opened_dialog(const std::string& _contact) = 0;

        virtual void get_stickers_meta(int64_t _seq, std::string_view _size) = 0;
        virtual void get_sticker(const int64_t _seq, const int32_t _set_id, const int32_t _sticker_id, const core::tools::filesharing_id& _fs_id, const core::sticker_size _size) = 0;
        virtual void get_sticker_cancel(std::vector<core::tools::filesharing_id> _fs_ids, core::sticker_size _size) = 0;
        virtual void get_stickers_pack_info(const int64_t _seq, const int32_t _set_id, const std::string& _store_id, const core::tools::filesharing_id& _file_id) = 0;
        virtual void add_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) = 0;
        virtual void remove_stickers_pack(const int64_t _seq, const int32_t _set_id, std::string _store_id) = 0;
        virtual void get_stickers_store(const int64_t _seq) = 0;
        virtual void search_stickers_store(const int64_t _seq, const std::string& _search_term) = 0;
        virtual void get_set_icon_big(const int64_t _seq, const int32_t _set_id) = 0;
        virtual void clean_set_icon_big(const int64_t _seq, const int32_t _set_id) = 0;
        virtual void set_sticker_order(const int64_t _seq, std::vector<int32_t> _values) = 0;
        virtual void get_fs_stickers_by_ids(const int64_t _seq, std::vector<std::pair<int32_t, int32_t>> _values) = 0;
        virtual void get_sticker_suggests(std::string _sn, std::string _text) = 0;
        virtual void get_smartreplies(std::string _aimid, int64_t _msgid, std::vector<smartreply::type> _types) = 0;
        virtual void clear_smartreply_suggests(const std::string_view _aimid) = 0;
        virtual void clear_smartreply_suggests_for_message(const std::string_view _aimid, const int64_t _msgid) = 0;
        virtual void load_cached_smartreplies(const std::string_view _aimid) const = 0;

        virtual void get_chat_home(int64_t _seq, const std::string& _tag) = 0;
        virtual void get_chat_info(int64_t _seq, const std::string& _aimid, const std::string& _stamp, int32_t _limit, bool _threads_auto_subscribe) = 0;
        virtual void get_chat_member_info(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _members) = 0;
        virtual void get_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, uint32_t _page_size) = 0;
        virtual void get_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void search_chat_members_page(int64_t _seq, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size) = 0;
        virtual void search_chat_members_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void get_chat_contacts(int64_t _seq, const std::string& _aimid, const std::string& _cursor, uint32_t _page_size) = 0;

        virtual void get_thread_subscribers_page(int64_t _seq, std::string_view _aimid, std::string_view _role, uint32_t _page_size) = 0;
        virtual void get_thread_subscribers_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void search_thread_subscribers_page(int64_t _seq, std::string_view _aimid, std::string_view _role, std::string_view _keyword, uint32_t _page_size) = 0;
        virtual void search_thread_subscribers_next_page(int64_t _seq, std::string_view _aimid, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void thread_autosubscribe(int64_t _seq, const std::string& _chatid, const bool _enable) = 0;

        virtual void check_themes_meta_updates(int64_t _seq) = 0;
        virtual void get_theme_wallpaper(const std::string_view _wp_id) = 0;
        virtual void get_theme_wallpaper_preview(const std::string_view _wp_id) = 0;
        virtual void set_user_wallpaper(const std::string_view _wp_id, tools::binary_stream _image) = 0;
        virtual void remove_user_wallpaper(const std::string_view _wp_id) = 0;

        virtual void resolve_pending(int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _contact, bool _approve) = 0;

        virtual void create_chat(int64_t _seq, const std::string& _aimid, const std::string& _name, std::vector<std::string> _members, core::wim::chat_params _params) = 0;

        virtual void mod_chat_params(int64_t _seq, const std::string& _aimid, core::wim::chat_params _params) = 0;
        virtual void mod_chat_name(int64_t _seq, const std::string& _aimid, const std::string& _name) = 0;
        virtual void mod_chat_about(int64_t _seq, const std::string& _aimid, const std::string& _about) = 0;
        virtual void mod_chat_rules(int64_t _seq, const std::string& _aimid, const std::string& _rules) = 0;

        virtual void set_attention_attribute(int64_t _seq, std::string _aimid, const bool _value, const bool _resume) = 0;

        virtual void block_chat_member(int64_t _seq, const std::string& _aimid, const std::string& _contact, bool _block, bool _remove_messages) = 0;
        virtual void set_chat_member_role(int64_t _seq, const std::string& _aimid, const std::string& _contact, const std::string& _role) = 0;
        virtual void pin_chat_message(int64_t _seq, const std::string& _aimid, int64_t _message, bool _is_unpin) = 0;

        virtual void get_mentions_suggests(int64_t _seq, const std::string& _aimid, const std::string& _pattern) = 0;

        // search functions
        virtual void search_dialogs_local(const std::string& _search_patterns, const std::vector<std::string>& _aimids) = 0;
        virtual void search_thread_feed_local(const std::string& _search_patterns, const std::string& _aimid) = 0;
        virtual void search_threads_local(const std::string& _search_patterns, const std::string& _aimid) = 0;
        virtual void search_contacts_local(const std::vector<std::vector<std::string>>& _search_patterns, int64_t _req_id, unsigned _fixed_patterns_count, const std::string& _pattern) = 0;
        virtual void setup_search_dialogs_params(int64_t _req_id, bool _in_thread) = 0;
        virtual void clear_search_dialogs_params(bool _in_thread) = 0;

        virtual void add_search_pattern_to_history(const std::string_view _search_pattern, const std::string_view _contact) = 0;
        virtual void remove_search_pattern_from_history(const std::string_view _search_pattern, const std::string_view _contact) = 0;


        // cl
        virtual std::string get_contact_friendly_name(const std::string& contact_login) = 0;
        virtual void hide_chat(const std::string& _contact) = 0;
        virtual void mute_chat(const std::string& _contact, bool _mute) = 0;
        virtual void add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message) = 0;
        virtual void remove_contact(int64_t _seq, const std::string& _aimid) = 0;
        virtual void rename_contact(int64_t _seq, const std::string& _aimid, const std::string& _friendly) = 0;
        virtual void ignore_contact(int64_t _seq, const std::string& _aimid, bool ignore) = 0;
        virtual void get_ignore_list(int64_t _seq) = 0;
        virtual void pin_chat(const std::string& _contact) = 0;
        virtual void unpin_chat(const std::string& _contact) = 0;
        virtual void send_notify_sms(std::string_view _contact) = 0;
        virtual void mark_unimportant(const std::string& _contact) = 0;
        virtual void remove_from_unimportant(const std::string& _contact) = 0;
        virtual void update_outgoing_msg_count(const std::string& _aimid, int _count) = 0;

#ifndef STRIP_VOIP

        // voip
        virtual void on_voip_call_start(const std::vector<std::string> &contacts, const voip_manager::CallStartParams &params);
        virtual void on_voip_add_window(voip_manager::WindowParams& windowParams);
        virtual void on_voip_remove_window(void* hwnd);
        virtual void on_voip_call_end(const std::string &call_id, const std::string &contact, bool busy, bool conference);
        virtual void on_voip_call_accept(const std::string &call_id, bool video);
        virtual void on_voip_call_stop();
        virtual void on_voip_proto_msg(bool allocate, const char* data, unsigned len, std::shared_ptr<auto_callback> _on_complete);
        virtual void on_voip_proto_ack(const voip_manager::VoipProtoMsg& msg, bool success);
        virtual void on_voip_update();
        virtual void on_voip_reset();

        virtual void on_voip_user_update_avatar(const std::string& contact, const unsigned char* data, unsigned size, unsigned avatar_h, unsigned avatar_w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_no_video(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_text(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_text_header(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);
        virtual void on_voip_user_update_avatar_background(const std::string& contact, const unsigned char* data, unsigned size, unsigned h, unsigned w, voip_manager::AvatarThemeType theme);

        virtual void on_voip_device_changed(std::string_view dev_type, const std::string& uid, bool force_reset);
        virtual void on_voip_devices_changed(DeviceClass deviceClass);

        virtual void on_voip_switch_media(bool video);
        virtual void on_voip_mute_switch();
        virtual void on_voip_set_mute(bool mute);
        virtual void on_voip_mute_incoming_call_sounds(bool mute);

        virtual void on_voip_peer_resolution_changed(const std::string& _contact, int _width, int _height);

        virtual bool has_created_call();

        virtual void post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) = 0;
        virtual void post_voip_alloc_to_server(const std::string& data) = 0;

        virtual std::shared_ptr<wim::wim_packet> prepare_voip_msg(const std::string& data) = 0;

        virtual std::shared_ptr<wim::wim_packet> prepare_voip_pac(const voip_manager::VoipProtoMsg& data) = 0;

        virtual void send_voip_calls_quality_report(int _score, const std::string& _survey_id,
            const std::vector<std::string>& _reasons, const std::string& _aimid);
#endif

        // files functions
        virtual void upload_file_sharing(
            const int64_t _seq,
            const std::string& _contact,
            const std::string& _file_name,
            std::shared_ptr<core::tools::binary_stream> _data,
            const std::string& _extension,
            const core::archive::quotes_vec& _quotes,
            const std::string& _description,
            const core::data::format& _description_format,
            const core::archive::mentions_map& _mentions,
            const std::optional<int64_t>& _duration,
            const bool _strip_exif,
            const int _orientation_tag) = 0;

        virtual void get_file_sharing_preview_size(
            const int64_t _seq,
            const std::string& _file_url,
            const int32_t _original_size) = 0;

        virtual void download_file_sharing_metainfo(
            const int64_t _seq,
            const core::tools::filesharing_id& _fs_id) = 0;

        virtual void download_file_sharing(
            const int64_t _seq,
            const std::string& _contact,
            const std::string& _file_url,
            const bool _force_request_metainfo,
            const std::string& _filename,
            const std::string& _download_dir,
            bool _raise_priority) = 0;

        virtual void download_image(
            const int64_t _seq,
            const std::string& _file_url,
            const std::string& _destination,
            const bool _download_preview,
            const int32_t _preview_width,
            const int32_t _preview_height,
            const bool _ext_resource,
            const bool _raise_priority) = 0;

        virtual void download_link_metainfo(
            const int64_t _seq,
            const std::string& _url,
            const int32_t _preview_width,
            const int32_t _preview_height,
            const bool _load_preview,
            std::string_view _log_str) = 0;

        virtual void cancel_loader_task(const std::string& _url, std::optional<int64_t> _seq) = 0;

        virtual void abort_file_sharing_download(const std::string& _url, std::optional<int64_t> _seq) = 0;

        virtual void abort_file_sharing_upload(
            const int64_t _seq,
            const std::string & _contact,
            const std::string &_process_seq) = 0;
        virtual void raise_download_priority(
            const int64_t _task_id) = 0;

        virtual void download_file(
            priority_t _priority,
            const std::string& _file_url,
            const std::string& _destination,
            std::string_view _normalized_url,
            bool _is_binary_data,
            std::function<void(bool)> _on_result) = 0;

        virtual void get_external_file_path(const int64_t _seq, const std::string& _url) = 0;

        virtual void contact_switched(const std::string &_contact_aimid) = 0;

        virtual void speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale) = 0;

        // search for contacts
        virtual void search_contacts_server(int64_t _seq, const std::string_view _keyword, const std::string_view _phone) = 0;
        virtual void syncronize_address_book(const int64_t _seq, const std::string& _keyword, const std::vector<std::string>& _phone) = 0;

        virtual void search_dialogs_one(const int64_t _seq, const std::string_view _keyword, const std::string_view _contact) = 0;
        virtual void search_dialogs_all(const int64_t _seq, const std::string_view _keyword) = 0;
        virtual void search_dialogs_continue(const int64_t _seq, const std::string_view _cursor, const std::string_view _contact) = 0;

        virtual void search_threads(const int64_t _seq, const std::string_view _keyword, const std::string_view _contact) = 0;
        virtual void search_threads_continue(const int64_t _seq, const std::string_view _cursor, const std::string_view _contact) = 0;

        virtual void update_profile(int64_t _seq, const std::vector<std::pair<std::string, std::string>>& _field) = 0;

        // live chats
        virtual void join_live_chat(int64_t _seq, const std::string& _stamp) = 0;

        virtual void set_avatar(const int64_t _seq, tools::binary_stream image, const std::string& _aimId, const bool _chat) = 0;

        virtual bool has_valid_login() const = 0;

        virtual void get_code_by_phone_call(const std::string& _ivr_url) = 0;

        virtual void get_voip_calls_quality_popup_conf(const int64_t _seq, const std::string& _locale, const std::string& _lang) = 0;
        virtual void get_logs_path(const int64_t _seq) = 0;
        virtual void remove_content_cache(const int64_t _seq) = 0;
        virtual void clear_avatars(const int64_t _seq) = 0;
        virtual void remove_omicron_stg(const int64_t _seq) = 0;

        virtual void report_contact(int64_t _seq, const std::string& _aimid, const std::string& _reason, const bool _ignore_and_close) = 0;
        virtual void report_stickerpack(const int32_t _id, const std::string& _reason) = 0;
        virtual void report_sticker(const std::string& _id, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) = 0;
        virtual void report_message(const int64_t _id, const std::string& _text, const std::string& _reason, const std::string& _aimid, const std::string& _chatId) = 0;

        virtual void user_accept_gdpr(int64_t _seq) = 0;

        virtual void send_stat() = 0;

        virtual void get_dialog_gallery(const int64_t _seq,
                                        const std::string& _aimid,
                                        const std::vector<std::string>& _type,
                                        const archive::gallery_entry_id _after,
                                        const int _page_size,
                                        const bool _download_holes) = 0;

        virtual void get_dialog_gallery_by_msg(const int64_t _seq, const std::string& _aimid, const std::vector<std::string>& _type, int64_t _msg_id) = 0;

        virtual void request_gallery_state(const std::string& _aimId) = 0;
        virtual void get_gallery_index(const std::string& _aimId, const std::vector<std::string>& _type, int64_t _msg, int64_t _seq) = 0;
        virtual void make_gallery_hole(const std::string& _aimid, int64_t _from, int64_t _till) = 0;
        virtual void stop_gallery_holes_downloading(const std::string& _aimId) = 0;

        virtual void request_memory_usage(const int64_t _seq) = 0;
        virtual void report_memory_usage(const int64_t _seq,
                                         memory_stats::request_id _req_handle,
                                         const memory_stats::partial_response& _partial_response) = 0;
        virtual void get_ram_usage(const int64_t _seq) = 0;

        virtual void make_archive_holes(const int64_t _seq, const std::string& _archive) = 0;
        virtual void invalidate_archive_history(const int64_t _seq, const std::string& _archive) = 0;
        virtual void invalidate_archive_data(const int64_t _seq, const std::string& _archive, std::vector<int64_t> _ids) = 0;
        virtual void invalidate_archive_data(const int64_t _seq, const std::string& _archive, int64_t _from, int64_t _before_count, int64_t _after_count) = 0;
        virtual void on_ui_activity(const int64_t _time) = 0;

        virtual void close_stranger(const std::string& _contact) = 0;

        virtual void on_local_pin_set(const std::string& _password) = 0;
        virtual void on_local_pin_entered(const int64_t _seq, const std::string& _password) = 0;
        virtual void on_local_pin_disable(const int64_t _seq) = 0;

        virtual void get_id_info(const int64_t _seq, const std::string_view _id) = 0;
        virtual void get_user_info(const int64_t _seq, const std::string& _aimid) = 0;
        virtual void get_user_last_seen(const int64_t _seq, std::vector<std::string> _ids) = 0;

        virtual void set_privacy_settings(const int64_t _seq, core::wim::privacy_settings _settings) = 0;
        virtual void get_privacy_settings(const int64_t _seq) = 0;

        virtual void on_nickname_check(const int64_t _seq, const std::string& _nickname, bool _set_nick) = 0;
        virtual void on_group_nickname_check(const int64_t _seq, const std::string& _nickname) = 0;
        virtual void on_get_common_chats(const int64_t _seq, const std::string& _sn) = 0;

        virtual void get_poll(const int64_t _seq, const std::string& _poll_id) = 0;
        virtual void vote_in_poll(const int64_t _seq, const std::string& _poll_id, const std::string& _answer_id) = 0;
        virtual void revoke_vote(const int64_t _seq, const std::string& _poll_id) = 0;
        virtual void stop_poll(const int64_t _seq, const std::string& _poll_id) = 0;

        virtual void create_task(int64_t _seq, const core::tasks::task_data& _task) = 0;
        virtual void edit_task(int64_t _seq, const core::tasks::task_change& _task) = 0;
        virtual void request_initial_tasks(int64_t _seq) = 0;
        virtual void update_task_last_used(int64_t _seq, const std::string& _task_id) = 0;

        virtual void group_subscribe(const int64_t _seq, std::string_view _stamp, std::string_view _aimid) = 0;
        virtual void cancel_group_subscription(const int64_t _seq, std::string_view _stamp, std::string_view _aimid) = 0;

        virtual void suggest_group_nick(const int64_t _seq, const std::string& _sn, bool _public) = 0;

        virtual void get_bot_callback_answer(const int64_t& _seq, const std::string_view _chat_id, const std::string_view _callback_data, int64_t _msg_id) = 0;
        virtual void start_bot(const int64_t _seq, std::string_view _nick, std::string_view _params) = 0;

        virtual void create_conference(int64_t _seq, std::string_view _name, bool _is_webinar, std::vector<std::string> _participants, bool _call_participants) = 0;

        virtual void get_sessions() = 0;
        virtual void reset_session(std::string_view _session_hash) = 0;

        virtual void update_parellel_packets_count(size_t _count) = 0;

        virtual void cancel_pending_join(std::string _sn) = 0;
        virtual void cancel_pending_invitation(std::string _contact, std::string _chat) = 0;

        virtual void get_reactions(const std::string& _chat_id, std::shared_ptr<archive::msgids_list> _msg_ids, bool _first_load) = 0;
        virtual void add_reaction(const int64_t _seq, int64_t _msg_id, const std::string& _chat_id, const std::string& _reaction, const std::vector<std::string>& _reactions_list) = 0;
        virtual void remove_reaction(const int64_t _seq, int64_t _msg_id, const std::string& _chat_id) = 0;
        virtual void list_reactions(const int64_t _seq,
                                    int64_t _msg_id,
                                    const std::string& _chat_id,
                                    const std::string& _reaction,
                                    const std::string& _newer_than,
                                    const std::string& _older_than,
                                    int64_t _limit) = 0;

        virtual void set_status(std::string_view _status, std::chrono::seconds _duration, std::string_view _description = std::string_view()) = 0;

        virtual void subscribe_status(std::vector<std::string> _contacts) = 0;
        virtual void unsubscribe_status(std::vector<std::string> _contacts) = 0;

        virtual void subscribe_call_room_info(const std::string& _room_id) = 0;
        virtual void unsubscribe_call_room_info(const std::string& _room_id) = 0;

        virtual void subscribe_task(const std::string& _task_id) = 0;
        virtual void unsubscribe_task(const std::string& _task_id) = 0;

        virtual void subscribe_thread_updates(std::vector<std::string> _thread_ids) = 0;
        virtual void unsubscribe_thread_updates(std::vector<std::string> _thread_ids) = 0;

        virtual void subscribe_filesharing_antivirus(const core::tools::filesharing_id& _filesharing_id) = 0;
        virtual void unsubscribe_filesharing_antivirus(const core::tools::filesharing_id& _filesharing_id) = 0;

        virtual void subscribe_mails_counter() = 0;
        virtual void unsubscribe_mails_counter() = 0;

        virtual void subscribe_tasks_counter() = 0;
        virtual void unsubscribe_tasks_counter() = 0;

        virtual void get_emoji(int64_t _seq, std::string_view _code, int _size) = 0;

        virtual void add_to_inviters_blacklist(std::vector<std::string> _contacts) = 0;
        virtual void remove_from_inviters_blacklist(std::vector<std::string> _contacts, bool _delete_all) = 0;
        virtual void search_inviters_blacklist(int64_t _seq, std::string_view _keyword, std::string_view _cursor, uint32_t _page_size) = 0;
        virtual void get_blacklisted_cl_inviters(std::string_view _cursor, uint32_t _page_size) = 0;

        virtual void misc_support(int64_t _seq, std::string_view _aimid, std::string_view _message, std::string_view _attachment_file_name, std::vector<std::string> _screenshot_file_name_list) = 0;

        virtual void add_thread(int64_t _seq, archive::thread_parent_topic _topic) = 0;
        virtual void subscribe_thread(int64_t _seq, std::string_view _thread_id) = 0;
        virtual void unsubscribe_thread(int64_t _seq, std::string_view _thread_id) = 0;
        virtual void get_thread_info(int64_t _seq, std::string_view _thread_id) = 0;
        virtual void get_threads_feed(int64_t _seq, const std::string& _cursor) = 0;

        virtual void add_chat_thread(std::string_view _parent_id, int64_t _msg_id, std::string_view _thread_id) = 0;
        virtual void remove_chat_thread(std::string_view _parent_id, std::string_view _thread_id) = 0;
        virtual void remove_chat_thread(std::string_view _parent_id, int64_t _msg_id) = 0;
        virtual void remove_chat_thread(std::string_view _parent_id) = 0;

        virtual void start_miniapp_session(std::string_view _miniapp_id) = 0;

        virtual void notify_history_droppped(const std::string& _aimid) = 0;
    };

}
