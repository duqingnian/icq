#include "stdafx.h"
#include "sync_ab.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

core::wim::sync_ab::sync_ab(wim_packet_params _params, const std::string& _keyword, const std::vector<std::string>& _phone)
    : robusto_packet(std::move(_params))
    , keyword_(_keyword)
    , phone_(_phone)
{
}

std::string_view core::wim::sync_ab::get_method() const
{
    return "contacts/sync";
}

int core::wim::sync_ab::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t core::wim::sync_ab::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    if (keyword_.empty() && phone_.empty())
        return -1;

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_update(rapidjson::Type::kArrayType);

    rapidjson::Value member_update(rapidjson::Type::kObjectType);

    member_update.AddMember("name", keyword_, a);

    if (!phone_.empty())
    {
        rapidjson::Value phones(rapidjson::Type::kArrayType);
        phones.Reserve(phone_.size(), a);
        for (const auto& phone : phone_)
            phones.PushBack(tools::make_string_ref(phone), a);
        member_update.AddMember("phoneList", std::move(phones), a);
    }

    node_update.PushBack(std::move(member_update), a);
    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("update", std::move(node_update), a);
    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!robusto_packet::params_.full_log_)
    {
        log_replace_functor f;
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        f.add_json_marker("keyword");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t core::wim::sync_ab::execute_request(const std::shared_ptr<core::http_request_simple>& request)
{
    url_ = request->get_url();

    if (auto error_code = get_error(request->post()))
        return *error_code;

    http_code_ = (uint32_t)request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}
