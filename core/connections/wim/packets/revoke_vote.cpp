#include "stdafx.h"

#include "../../../http_request.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

#include "revoke_vote.h"


namespace core::wim
{

revoke_vote::revoke_vote(wim_packet_params _params, const std::string &_poll_id)
    : robusto_packet(std::move(_params))
    , poll_id_(_poll_id)
{

}

std::string_view revoke_vote::get_method() const
{
    return "poll/revoke";
}

int revoke_vote::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t revoke_vote::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("id", poll_id_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

}
