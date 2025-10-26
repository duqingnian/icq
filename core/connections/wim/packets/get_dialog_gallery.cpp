#include "stdafx.h"

#include "get_dialog_gallery.h"
#include "http_request.h"
#include "../log_replace_functor.h"

using namespace core;
using namespace core::wim;


get_dialog_gallery::get_dialog_gallery(wim_packet_params _params,
                                       const std::string& _aimid,
                                       const std::string& _path_version,
                                       const core::archive::gallery_entry_id& _from,
                                       const core::archive::gallery_entry_id& _till,
                                       const int _count, bool _parse_for_pacthes)
    : robusto_packet(std::move(_params))
    , gallery_(_aimid)
    , aimid_(_aimid)
    , path_version_(_path_version)
    , from_(_from)
    , till_(_till)
    , count_(_count)
    , parse_for_patches_(_parse_for_pacthes)
{
}

get_dialog_gallery::~get_dialog_gallery() = default;

const core::archive::gallery_storage& get_dialog_gallery::get_gallery() const
{
    return gallery_;
}

int32_t get_dialog_gallery::init_request(const std::shared_ptr<core::http_request_simple>& _request)
{
    rapidjson::Document doc(rapidjson::Type::kObjectType);
    auto& a = doc.GetAllocator();

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", aimid_, a);
    node_params.AddMember("galleryPatchVersion", path_version_.empty() ? "init" : path_version_, a);
    rapidjson::Value node_subreqs(rapidjson::Type::kArrayType);
    node_subreqs.Reserve(1, a);
    {
        rapidjson::Value node(rapidjson::Type::kObjectType);
        if (!from_.empty())
        {
            rapidjson::Value node_id(rapidjson::Type::kObjectType);
            node_id.AddMember("mid", from_.msg_id_, a);
            node_id.AddMember("seq", from_.seq_, a);
            node.AddMember("fromEntryId", std::move(node_id), a);
        }
        else
        {
            node.AddMember("fromEntryId", "max", a);
        }
        if (!till_.empty())
        {
            rapidjson::Value node_id(rapidjson::Type::kObjectType);
            node_id.AddMember("mid", till_.msg_id_, a);
            node_id.AddMember("seq", till_.seq_, a);
            node.AddMember("tillEntryId", std::move(node_id), a);
        }
        node.AddMember("subreqId", "first", a);
        node.AddMember("entryCount", -1 * count_, a);
        node_subreqs.PushBack(std::move(node), a);
    }

    node_params.AddMember("subreqs", std::move(node_subreqs), a);

    doc.AddMember("params", std::move(node_params), a);

    setup_common_and_sign(doc, a, _request, get_method());

    if (!params_.full_log_)
    {
        log_replace_functor f;
        f.add_message_markers();
        f.add_json_marker("aimsid", aimsid_range_evaluator());
        _request->set_replace_log_function(f);
    }

    return 0;
}

int32_t get_dialog_gallery::parse_results(const rapidjson::Value &_node_results)
{
    gallery_.set_my_aimid(get_params().aimid_);
    return gallery_.unserialize(_node_results, parse_for_patches_);
}

priority_t get_dialog_gallery::get_priority() const
{
    return priority_protocol();
}

int core::wim::get_dialog_gallery::minimal_supported_api_version() const
{
    return core::urls::api_version::instance().minimal_supported();
}

std::string_view get_dialog_gallery::get_method() const
{
    return "getEntryGallery";
}
