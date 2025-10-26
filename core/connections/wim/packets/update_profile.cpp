#include "stdafx.h"
#include "update_profile.h"

#include "../../../http_request.h"
#include "../../../tools/system.h"
#include "../../urls_cache.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

update_profile::update_profile(
    wim_packet_params _params,
    const std::vector<std::pair<std::string, std::string>>& _fields)
    :    wim_packet(std::move(_params)),
    fields_(_fields)
{
}

update_profile::~update_profile()
{
}

std::string_view update_profile::get_method() const
{
    return "updateMemberDir";
}

int core::wim::update_profile::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

int32_t update_profile::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    im_assert(!fields_.empty());
    if (fields_.empty())
        return 0;

    std::stringstream ss_url;

    ss_url << urls::get_url(urls::url_type::wim_host) << "memberDir/update" <<
        "?f=json" <<
        "&aimsid=" << get_params().aimsid_ <<
        "&r=" << core::tools::system::generate_guid();

    std::for_each(fields_.begin(), fields_.end(), [&ss_url](const std::pair<std::string, std::string>& item){ ss_url << "&set=" << escape_symbols(item.first + '=' + item.second); });
    _request->set_url(ss_url.str());
    _request->set_normalized_url(get_method());
    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t update_profile::parse_response_data(const rapidjson::Value& /*_data*/)
{
    return 0;
}
