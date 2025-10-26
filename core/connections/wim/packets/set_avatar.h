#ifndef __WIM_SET_AVATAR_H_
#define __WIM_SET_AVATAR_H_

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
        class set_avatar : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t execute_request(const std::shared_ptr<core::http_request_simple>& request) override;
            virtual int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& response) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string aimid_;
            bool chat_;
            tools::binary_stream image_;

            std::string id_;

        public:
            set_avatar(wim_packet_params _params, tools::binary_stream _image, const std::string& _aimId, const bool _chat);
            virtual ~set_avatar();

            inline const std::string& get_id() const { return id_; }
            bool is_post() const override { return true; }
            bool support_async_execution() const override { return false; }
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}

#endif// __WIM_SET_AVATAR_H_
