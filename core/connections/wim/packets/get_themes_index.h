#ifndef get_themes_index_hpp
#define get_themes_index_hpp

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
    namespace tools
    {
        class binary_stream;
    }

    namespace wim
    {
        class get_themes_index : public wim_packet
        {
        private:
            std::string etag_;
            std::string header_etag_;

            std::shared_ptr<core::tools::binary_stream> response_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response(const std::shared_ptr<core::tools::binary_stream>& _response) override;

        public:
            get_themes_index(wim_packet_params _params, const std::string &etag);
            virtual ~get_themes_index();

            std::shared_ptr<core::tools::binary_stream> get_response() const;
            const std::string& get_header_etag() const;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}

#endif /* get_themes_index_hpp */
