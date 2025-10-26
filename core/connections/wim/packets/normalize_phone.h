#ifndef __NORMALIZE_PHONE_H_
#define __NORMALIZE_PHONE_H_

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
    class phone_info;

    namespace wim
    {
        class normalize_phone : public wim_packet
        {
            const std::string	country_;
            const std::string	phone_;

            std::string			normalized_phone_;
            bool				sms_enabled_;

        public:
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;
            int32_t on_http_client_error() override;

            normalize_phone(wim_packet_params _params, const std::string& _country, const std::string& _phone);
            virtual ~normalize_phone();

            const std::string& get_normalized_phone() const { return normalized_phone_; }
            bool get_sms_enabled() const { return sms_enabled_; }
            bool is_valid() const override { return true; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }
}


#endif //__NORMALIZE_PHONE_H_