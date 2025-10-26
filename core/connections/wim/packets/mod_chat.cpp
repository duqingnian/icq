#include "stdafx.h"
#include "mod_chat.h"
#include "../chat_params.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../../../common.shared/json_helper.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

mod_chat::mod_chat(
    wim_packet_params _params,
    const std::string& _aimid)
    : robusto_packet(std::move(_params))
    , aimid_(_aimid)
{
}

mod_chat::~mod_chat() = default;

chat_params& mod_chat::get_chat_params()
{
    return chat_params_;
}

void mod_chat::set_chat_params(chat_params _chat_params)
{
    chat_params_ = std::move(_chat_params);
}

std::string_view mod_chat::get_method() const
{
    return "modChat";
}

int core::wim::mod_chat::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t mod_chat::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("sn", aimid_, a);
    if (chat_params_.get_name())
        node_params.AddMember("name", *chat_params_.get_name(), a);
    if (chat_params_.get_about())
        node_params.AddMember("about", *chat_params_.get_about(), a);
    if (chat_params_.get_rules())
        node_params.AddMember("rules", *chat_params_.get_rules(), a);
    if (chat_params_.get_public())
        node_params.AddMember("public", *chat_params_.get_public(), a);
    if (chat_params_.get_join())
        node_params.AddMember("joinModeration", *chat_params_.get_join(), a);
    if (chat_params_.get_stamp())
        node_params.AddMember("stamp", *chat_params_.get_stamp(), a);
    if (chat_params_.get_trust_required())
        node_params.AddMember("trustRequired", *chat_params_.get_trust_required(), a);
    if (chat_params_.get_threads_enabled())
        node_params.AddMember("threadsEnabled", *chat_params_.get_threads_enabled(), a);

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

int32_t mod_chat::parse_response_data(const rapidjson::Value& _data)
{
    return 0;
}
