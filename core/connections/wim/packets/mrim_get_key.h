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
        class mrim_get_key : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& response) override;

            std::string email_;
            std::string mrim_key_;

        public:

            mrim_get_key(
                wim_packet_params _params,
                const std::string& _email);

            virtual ~mrim_get_key();

            const std::string& get_mrim_key() const { return mrim_key_; }

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}