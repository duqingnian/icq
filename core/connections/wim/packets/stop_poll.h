#pragma once

#include "../robusto_packet.h"

namespace core::wim
{

class stop_poll : public robusto_packet
{
public:
    stop_poll(wim_packet_params _params, const std::string& _poll_id);
    std::string_view get_method() const override;
    int minimal_supported_api_version() const override;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;

    std::string poll_id_;
};

}

