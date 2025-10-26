#include "stdafx.h"

#include "set_privacy_settings.h"
#include "../../../http_request.h"
#include "../log_replace_functor.h"

namespace core::wim
{
    set_privacy_settings::set_privacy_settings(wim_packet_params _params, privacy_settings _settings)
        : robusto_packet(std::move(_params))
        , settings_(std::move(_settings))
    {
    }

    std::string_view set_privacy_settings::get_method() const
    {
        return "updatePrivacySettings";
    }

    int set_privacy_settings::minimal_supported_api_version() const
    {
        return core::urls::api_version::instance().minimal_supported();
    }

    int32_t set_privacy_settings::init_request(const std::shared_ptr<core::http_request_simple>& _request)
    {
        rapidjson::Document doc(rapidjson::Type::kObjectType);
        auto& a = doc.GetAllocator();

        rapidjson::Value node_params(rapidjson::Type::kObjectType);

        im_assert(settings_.has_values_set());
        settings_.serialize(node_params, a);

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
}