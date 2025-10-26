#ifndef __WIM_CLIENT_LOGIN_H_
#define __WIM_CLIENT_LOGIN_H_

#pragma once

#include "../wim_packet.h"

#define WIM_APP_VER "5000"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    enum class token_type;

    namespace wim
    {
        class client_login : public wim_packet
        {
            std::string login_;
            std::string password_;

            std::string a_token_;
            uint32_t expired_in_;
            uint32_t host_time_;
            int64_t time_offset_;
            token_type token_type_;

            std::string product_guid_8x_;
            bool need_fill_profile_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;
            int32_t on_response_error_code() override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t on_empty_data() override;

        public:
            const std::string& get_a_token() const { return a_token_; }
            uint32_t get_expired_in() const { return expired_in_; }
            uint32_t get_host_time() const { return host_time_; }
            int64_t get_time_offset() const { return time_offset_; }

            void set_product_guid_8x(const std::string& _guid);

            const bool get_need_fill_profile() const { return need_fill_profile_; }
            bool is_valid() const override { return true; }

            priority_t get_priority() const override { return priority_protocol(); }
            bool is_post() const override { return true; }

            std::string_view get_method() const override;

            client_login(
                wim_packet_params params,
                const std::string& login,
                const std::string& password,
                const token_type token_type = token_type::basic);

            virtual ~client_login();

            int minimal_supported_api_version() const override;
        };
    }
}


#endif //__WIM_CLIENT_LOGIN_H_
