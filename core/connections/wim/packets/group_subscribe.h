#pragma once

#include "../robusto_packet.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class group_subscribe : public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;

            std::string stamp_;
            std::string aimid_;
            int32_t resubscribe_in_;

        public:
            group_subscribe(wim_packet_params _params, std::string_view _stamp, std::string_view _aimid);
            int32_t get_resubscribe_in() const;
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;

            const std::string& get_param() const noexcept { return stamp_.empty() ? aimid_ : stamp_; }
        };
    }
}
