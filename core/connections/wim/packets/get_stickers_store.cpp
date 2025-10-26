#include "stdafx.h"
#include "get_stickers_store.h"
#include "../../../http_request.h"
#include "../../../tools/hmac_sha_base64.h"
#include "../../../core.h"
#include "../../../tools/system.h"
#include "../../../utils.h"
#include "../../urls_cache.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace wim;

get_stickers_store_packet::get_stickers_store_packet(wim_packet_params _params, std::string _search_term)
    : wim_packet(std::move(_params))
    , search_term_(std::move(_search_term))
{
}

get_stickers_store_packet::~get_stickers_store_packet()
{
}

int32_t get_stickers_store_packet::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    std::map<std::string, std::string> params;

    const time_t ts = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::stringstream ss_host;
    ss_host << urls::get_url(urls::url_type::stickers_store_host) << std::string_view("/store/showcase");

    params["aimsid"] = escape_symbols(params_.aimsid_);
    params["f"] = "json";
    params["k"] = params_.dev_id_;
    params["ts"] = std::to_string((int64_t) ts);
    params["r"] = core::tools::system::generate_guid();

    params["client"] = core::utils::get_client_string();

    params["lang"] = params_.locale_;

    params["platform"] = utils::get_protocol_platform_string();

    if (!search_term_.empty())
        params["search"] = escape_symbols(search_term_);

    std::stringstream ss_url;
    ss_url << ss_host.str() << '?' << format_get_params(params);

    _request->set_url(ss_url.str());

    _request->set_normalized_url(get_method());

    _request->set_keep_alive();

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_marker("a");
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_stickers_store_packet::parse_response(const std::shared_ptr<core::tools::binary_stream>& _response)
{
    if (!_response->available())
        return wpie_http_empty_response;

    response_ = _response;

    return 0;
}

const std::shared_ptr<core::tools::binary_stream>& get_stickers_store_packet::get_response() const
{
    return response_;
}

priority_t get_stickers_store_packet::get_priority() const
{
    return packets_priority_high();
}

std::string_view get_stickers_store_packet::get_method() const
{
    return !search_term_.empty() ? "stickersStoreShowcaseSearch" : "stickersStoreShowcase";
}

int core::wim::get_stickers_store_packet::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}
