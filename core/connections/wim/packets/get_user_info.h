#pragma once

#include "../../../namespaces.h"

#include "../robusto_packet.h"
#include "../persons_packet.h"
#include "../user_info.h"

CORE_WIM_NS_BEGIN

class get_user_info: public robusto_packet, public persons_packet
{
public:
    get_user_info(wim_packet_params _params, const std::string& _contact_aimid);
    user_info get_info() const;
    const std::shared_ptr<core::archive::persons_map>& get_persons() const override;
    priority_t get_priority() const override { return top_priority(); }
    std::string_view get_method() const override;
    int minimal_supported_api_version() const override;

private:
    int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
    int32_t parse_results(const rapidjson::Value& _data) override;

private:
    const std::string contact_aimid_;
    user_info info_;
    std::shared_ptr<core::archive::persons_map> persons_;
};

CORE_WIM_NS_END