#ifndef __STARTSESSION_H_
#define __STARTSESSION_H_

#pragma once

#include "../wim_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}

namespace core
{
    namespace wim
    {
        class start_session : public wim_packet
        {
            int32_t init_request_full_start_session(const std::shared_ptr<core::http_request_simple>& _request);
            int32_t init_request_short_start_session(const std::shared_ptr<core::http_request_simple>& _request);

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;
            void parse_response_data_on_error(const rapidjson::Value& _data) override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t on_response_error_code() override;

            std::string uniq_device_id_;
            std::string locale_;
            bool is_ping_;

            bool need_relogin_;
            bool correct_ts_;

            std::string fetch_url_;
            std::string aimsid_;
            std::string aimid_;

            std::chrono::milliseconds timeout_;
            std::function<bool(std::chrono::milliseconds)> wait_function_;

            int64_t ts_;

        public:

            bool need_relogin() const { return need_relogin_; }
            bool need_correct_ts() const { return correct_ts_; }

            const std::string& get_fetch_url() const { return fetch_url_; }
            const std::string& get_aimsid() const { return aimsid_; }
            time_t get_ts() const { return ts_; }
            const std::string& get_aimid() const { return aimid_; }

            start_session(
                wim_packet_params params,
                const bool _is_ping,
                const std::string& _uniq_device_id,
                const std::string& _locale,
                std::chrono::milliseconds _timeout,
                std::function<bool(std::chrono::milliseconds)> _wait_function);

            virtual ~start_session();

            priority_t get_priority() const override;
            bool is_post() const override { return true; }

            std::string_view get_method() const override;
            bool support_self_resending() const override { return true; }
            int minimal_supported_api_version() const override;
        };
    }
}

#endif //__STARTSESSION_H_
