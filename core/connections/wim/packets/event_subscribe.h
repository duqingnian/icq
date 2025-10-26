#pragma once

#include "../robusto_packet.h"
#include "subscriptions/subscription.h"

namespace core
{
    class http_request_simple;

    namespace wim
    {
        class event_subscribe : public robusto_packet
        {
            std::unordered_map<subscriptions::type, subscriptions::subscr_ptr_v> subscriptions_;

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

        public:
            event_subscribe(wim_packet_params _params, subscriptions::subscr_ptr_v _subscriptions);
            ~event_subscribe() = default;

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };
    }
}