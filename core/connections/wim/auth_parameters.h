#ifndef __AUTH_PARAMETERS_H_
#define __AUTH_PARAMETERS_H_

#pragma once

namespace core
{
    namespace smartreply
    {
        enum class type;
    }

    class coll_helper;
    namespace wim
    {
        typedef std::chrono::time_point<std::chrono::system_clock, std::chrono::system_clock::duration> timepoint;

        struct miniapp_auth_parameters
        {
            std::string aimsid_;
        };

        struct auth_parameters
        {
            std::string aimid_;
            std::optional<std::string> o2token_;
            std::optional<std::string> o2refresh_;
            std::string a_token_;
            std::string dev_id_;
            std::string aimsid_;
            std::string locale_;
            std::string password_md5_;
            std::string product_guid_8x_;
            std::string agent_token_;

            time_t exipired_in_;
            time_t time_offset_;
            time_t time_offset_local_;

            std::string robusto_token_;
            int32_t robusto_client_id_;
            std::string version_;

            bool serializable_;

            std::string login_;
            std::string fetch_url_;

            std::map<std::string, miniapp_auth_parameters> miniapps_;

            auth_parameters();
            bool is_valid_agent_token() const;
            bool is_valid_token() const;
            bool is_valid_o2token() const;
            bool is_valid_md5() const;
            bool is_valid() const;
            bool is_robusto_valid() const;
            bool is_valid_miniapp(const std::string& _id) const;

            void reset_robusto();
            void clear();

            void serialize(core::tools::binary_stream& _stream) const;
            bool unserialize(core::tools::binary_stream& _stream);

            bool unserialize(const rapidjson::Value& _node);
            void serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const;

            bool unserialize(coll_helper& _params);
        };

        struct fetch_parameters
        {
            std::string fetch_url_;
            timepoint next_fetch_time_;
            std::chrono::seconds next_fetch_timeout_;
            time_t last_successful_fetch_;
            std::vector<smartreply::type> suggest_types_;
            bool hotstart_ = false;

            fetch_parameters();
            bool is_valid() const;
            void serialize(core::tools::binary_stream& _stream) const;
            bool unserialize(core::tools::binary_stream& _stream, const std::string& _aimsid);
        };

        using url_range_evaluator = std::function<std::pair<size_t, size_t>(std::string_view s)>;
        std::string hide_url_param(const std::string& _url, const std::string& _param, url_range_evaluator _range_eval = nullptr);
        std::string recover_hidden_url_param(const std::string& _url, const std::string& _param, const std::string& _value);
    }
}




#endif //__AUTH_PARAMETERS_H_
