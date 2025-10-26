#ifndef __GET_FILE_META_INFO_H_
#define __GET_FILE_META_INFO_H_

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
        class speech_to_text : public wim_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;
            int32_t execute_request(const std::shared_ptr<core::http_request_simple>& request) override;

            std::string Url_;
            std::string Locale_;
            std::string Text_;
            int32_t Comeback_;

        public:

            speech_to_text(
                wim_packet_params _params,
                const std::string& _url,
                const std::string& _locale);

            virtual ~speech_to_text();

            const std::string& get_text() const;
            int32_t get_comeback() const;

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}

#endif //__GET_FILE_META_INFO_H_