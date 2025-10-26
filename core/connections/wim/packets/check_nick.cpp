#include "stdafx.h"
#include "check_nick.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

check_nick::check_nick(wim_packet_params _params, const std::string& _nick, bool _set_nick)
    : robusto_packet(std::move(_params))
    , nickname_(_nick)
    , set_nick_(_set_nick)
{
}

std::string_view check_nick::get_method() const
{
    return set_nick_ ? "setNick" : "checkNick";
}

int core::wim::check_nick::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t check_nick::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("nick", nickname_, a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t check_nick::on_response_error_code()
{
    if (status_code_ == 40000)
        return wpie_error_nickname_bad_value;
    else if (status_code_ == 40600)
        return wpie_error_nickname_already_used;
    else if (status_code_ == 40100)
        return wpie_error_nickname_not_allowed;

    return wpie_error_message_unknown;
}
