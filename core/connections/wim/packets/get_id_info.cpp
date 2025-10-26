#include "stdafx.h"

#include "get_id_info.h"
#include "http_request.h"
#include "../log_replace_functor.h"

using namespace core::wim;

get_id_info::get_id_info(core::wim::wim_packet_params _params, const std::string_view _id)
    : robusto_packet(std::move(_params))
    , id_(_id)
{
    im_assert(!_id.empty());
}

const id_info_response& get_id_info::get_response() const
{
    return response_;
}

const std::shared_ptr<core::archive::persons_map>& core::wim::get_id_info::get_persons() const
{
    return persons_;
}

std::string_view get_id_info::get_method() const
{
    return "getIdInfo";
}

int core::wim::get_id_info::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t get_id_info::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);
    node_params.AddMember("id", id_, a);

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

int32_t get_id_info::parse_results(const rapidjson::Value& _node_results)
{
    const auto res = response_.unserialize(_node_results);

    if (response_.type_ != id_info_response::id_type::invalid)
    {
        core::archive::person p;
        p.friendly_ = response_.name_;
        p.nick_ = response_.nick_;
        p.first_name_ = response_.first_name_;
        p.middle_name_ = response_.middle_name_;
        p.last_name_ = response_.last_name_;

        persons_ = std::make_shared<core::archive::persons_map>();
        persons_->insert({ response_.sn_, std::move(p) });
    }

    return res;
}
