#pragma once

namespace core
{
    enum class ext_url_config_error;
}

namespace config
{
    namespace hosts
    {
        enum class host_url_type
        {
            base,
            base_binary,

            files_parse,
            stickerpack_share,
            miniapp_sharing,
            profile,

            mail_auth,
            mail_redirect,
            mail_win,
            mail_read,

            app_update,

            privacy_policy_url,
            terms_of_use_url,

            oauth_url,
            token_url,
            redirect_uri,

            // TODO: remove when deprecated
            di,
            di_dark,
            tasks,
            calendar,
            mail,
            tarm_mail,
            tarm_cloud,
            tarm_calls,
        };

        struct dns_entry
        {
            std::string ip_;
            bool ip_mode_ = true;
        };

        [[nodiscard]] std::string get_host_url(host_url_type _type);

        std::optional<dns_entry> resolve_host(const std::string& _url);
        std::string format_resolve_host_str(std::string_view _host, const dns_entry& _dns_entry);

        bool is_valid();
        bool load_config();
        void load_dns_cache();
        void clear_external_config();

        void resolve_hosts(bool _load = false);
        void switch_to_dns_mode(std::string_view _url, int _error);
        void switch_to_ip_mode(std::string_view _url, int _error);

        using load_callback_t = std::function<void(core::ext_url_config_error _error, std::string _host)>;
        void load_external_config_from_url(std::string_view _url, load_callback_t _callback);
        void load_external_url_config(std::vector<std::string>&& _hosts, load_callback_t&& _callback);
        void send_config_to_gui();
    }
}
