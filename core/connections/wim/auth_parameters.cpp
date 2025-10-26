#include "stdafx.h"
#include "auth_parameters.h"
#include "wim_packet.h"
#include "../../../corelib/collection_helper.h"
#include "../../utils.h"
#include "../../../common.shared/json_helper.h"
#include "log_replace_functor.h"

using namespace core;
using namespace wim;

core::wim::auth_parameters::auth_parameters()
    : dev_id_(utils::get_dev_id())
    , exipired_in_(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()))
    , time_offset_(0)
    , time_offset_local_(0)
    , robusto_client_id_(-1)
    , serializable_(true)
{
}

bool core::wim::auth_parameters::is_valid_agent_token() const
{
    return (!agent_token_.empty() && !login_.empty() && !product_guid_8x_.empty());
}

bool core::wim::auth_parameters::is_valid_token() const
{
    return (!a_token_.empty() && !dev_id_.empty());
}

bool core::wim::auth_parameters::is_valid_o2token() const
{
    return (o2token_ && !dev_id_.empty());
}

bool core::wim::auth_parameters::is_valid_md5() const
{
    return (!password_md5_.empty() && !login_.empty());
}

bool core::wim::auth_parameters::is_valid() const
{
    return (is_valid_token() || is_valid_md5() || is_valid_agent_token() || is_valid_o2token());
}

void core::wim::auth_parameters::reset_robusto()
{
    robusto_token_.clear();
    robusto_client_id_ = -1;
}

void core::wim::auth_parameters::clear()
{
    aimid_.clear();
    o2token_.reset();
    a_token_.clear();
    dev_id_.clear();
    aimsid_.clear();
    version_.clear();
    product_guid_8x_.clear();
    agent_token_.clear();
    miniapps_.clear();

    reset_robusto();
}

bool core::wim::auth_parameters::is_robusto_valid() const
{
    return (
        !robusto_token_.empty() &&
        (robusto_client_id_ > 0));
}

bool core::wim::auth_parameters::is_valid_miniapp(const std::string& _id) const
{
    if (const auto iter = miniapps_.find(_id); iter != miniapps_.end())
        return !iter->second.aimsid_.empty();

    return false;
}

enum auth_param_type
{
    apt_aimid = 1,
    apt_a_token = 2,
    apt_session_key = 3,
    apt_dev_id = 4,
    apt_aimsid = 5,
    apt_exipired_in = 7,
    apt_time_offset = 8,
    apt_version = 11,
    apt_locale = 12,
    apt_time_offset_local = 13,
    apt_o2token = 14,
    apt_o2refresh = 15,

    apt_robusto_token = 100,
    apt_robusto_client_id = 101,

    apt_miniapps = 200,
    apt_miniapp_id = 201,
    apt_miniapp_aimsid = 202,
};

enum fetch_param_type
{
    fpt_fetch_url = 1,
    fpt_next_fetch_time = 2,
    fpt_last_successful_fetch = 3
};

void core::wim::auth_parameters::serialize(core::tools::binary_stream& _stream) const
{
    if (!serializable_)
        return;

    core::tools::tlvpack pack;

    core::tools::binary_stream temp_stream;

    pack.push_child(core::tools::tlv(apt_aimid, aimid_));
    pack.push_child(core::tools::tlv(apt_a_token, a_token_));
    if (o2token_)
        pack.push_child(core::tools::tlv(apt_o2token, *o2token_));
    if (o2refresh_)
        pack.push_child(core::tools::tlv(apt_o2refresh, *o2refresh_));
    pack.push_child(core::tools::tlv(apt_dev_id, dev_id_));
    pack.push_child(core::tools::tlv(apt_aimsid, aimsid_));
    pack.push_child(core::tools::tlv(apt_exipired_in, (int64_t) exipired_in_));
    pack.push_child(core::tools::tlv(apt_time_offset, (int64_t) time_offset_));
    pack.push_child(core::tools::tlv(apt_time_offset_local, (int64_t)time_offset_local_));
    pack.push_child(core::tools::tlv(apt_version, version_));
    pack.push_child(core::tools::tlv(apt_locale, locale_));

    pack.push_child(core::tools::tlv(apt_robusto_token, robusto_token_));
    pack.push_child(core::tools::tlv(apt_robusto_client_id, robusto_client_id_));

    if (!miniapps_.empty())
    {
        core::tools::tlvpack miniapps_pack;

        for (const auto& [id, params] : miniapps_)
        {
            core::tools::tlvpack app_pack;

            app_pack.push_child(core::tools::tlv(apt_miniapp_id, id));
            app_pack.push_child(core::tools::tlv(apt_miniapp_aimsid, params.aimsid_));

            miniapps_pack.push_child(core::tools::tlv(0, app_pack));
        }

        pack.push_child(core::tools::tlv(apt_miniapps, miniapps_pack));
    }

    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool core::wim::auth_parameters::unserialize(core::tools::binary_stream& _stream)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_aimid = tlv_pack_childs.get_item(apt_aimid);
    auto tlv_a_token = tlv_pack_childs.get_item(apt_a_token);
    auto tlv_o2token = tlv_pack_childs.get_item(apt_o2token);
    auto tlv_o2refresh = tlv_pack_childs.get_item(apt_o2refresh);
    auto tlv_dev_id = tlv_pack_childs.get_item(apt_dev_id);
    auto tlv_aim_sid = tlv_pack_childs.get_item(apt_aimsid);
    auto tlv_exipired_in = tlv_pack_childs.get_item(apt_exipired_in);
    auto tlv_time_offset = tlv_pack_childs.get_item(apt_time_offset);
    auto tlv_time_offset_local = tlv_pack_childs.get_item(apt_time_offset_local);
    auto tlv_version = tlv_pack_childs.get_item(apt_version);
    auto tlv_locale = tlv_pack_childs.get_item(apt_locale);

    auto tlv_robusto_token = tlv_pack_childs.get_item(apt_robusto_token);
    auto tlv_robusto_client_id = tlv_pack_childs.get_item(apt_robusto_client_id);

    if (
        !tlv_aimid ||
        (!tlv_a_token && !tlv_o2token) ||
        !tlv_dev_id ||
        !tlv_aim_sid ||
        !tlv_exipired_in ||
        !tlv_time_offset)
        return false;

    aimid_ = tlv_aimid->get_value<std::string>();
    if (tlv_a_token)
        a_token_ = tlv_a_token->get_value<std::string>();
    if (tlv_o2token)
        o2token_ = tlv_o2token->get_value<std::string>();
    if (tlv_o2refresh)
        o2refresh_ = tlv_o2refresh->get_value<std::string>();
    dev_id_ = tlv_dev_id->get_value<std::string>();
    aimsid_ = tlv_aim_sid->get_value<std::string>();
    exipired_in_ = tlv_exipired_in->get_value<int64_t>(0);
    time_offset_ = tlv_time_offset->get_value<int64_t>(0);

    if (tlv_robusto_token)
        robusto_token_ = tlv_robusto_token->get_value<std::string>();

    if (tlv_robusto_client_id)
        robusto_client_id_ = tlv_robusto_client_id->get_value<int32_t>(-1);

    if (tlv_version)
        version_ = tlv_version->get_value<std::string>();

    if (tlv_locale)
        locale_ = tlv_locale->get_value<std::string>();

    if (tlv_time_offset_local)
        time_offset_local_ = tlv_time_offset_local->get_value<int64_t>(0);

    if (auto tlv_miniapps = tlv_pack_childs.get_item(apt_miniapps))
    {
        core::tools::tlvpack tlv_pack_heads;
        if (tlv_pack_heads.unserialize(tlv_miniapps->get_value<core::tools::binary_stream>()))
        {
            auto val = tlv_pack_heads.get_first();

            while (val)
            {
                core::tools::tlvpack tlv_pack_head;

                tlv_pack_head.unserialize(val->get_value<core::tools::binary_stream>());

                auto tlv_id = tlv_pack_head.get_item(apt_miniapp_id);
                auto tlv_aimsid = tlv_pack_head.get_item(apt_miniapp_aimsid);

                if (tlv_id && tlv_aimsid)
                {
                    auto id = tlv_id->get_value<std::string>();
                    auto aimsid = tlv_aimsid->get_value<std::string>();
                    if (!id.empty() && !aimsid.empty())
                        miniapps_[std::move(id)] = { std::move(aimsid) };
                }

                val = tlv_pack_heads.get_next();
            }
        }
    }

    return true;
}

bool core::wim::auth_parameters::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("aimid");
    if (iter_aimid == _node.MemberEnd() || !iter_aimid->value.IsString())
        return false;

    aimid_ = rapidjson_get_string(iter_aimid->value);

    auto iter_atoken = _node.FindMember("atoken");
    if (iter_atoken != _node.MemberEnd() && iter_atoken->value.IsString())
        a_token_ = rapidjson_get_string(iter_atoken->value);

    auto iter_agenttoken = _node.FindMember("agenttoken");
    if (iter_agenttoken != _node.MemberEnd() && iter_agenttoken->value.IsString())
        agent_token_ = rapidjson_get_string(iter_agenttoken->value);

    auto iter_guid = _node.FindMember("productguid");
    if (iter_guid != _node.MemberEnd() && iter_guid->value.IsString())
        product_guid_8x_ = rapidjson_get_string(iter_guid->value);

    auto iter_devid = _node.FindMember("devid");
    if (iter_devid != _node.MemberEnd() && iter_devid->value.IsString())
        dev_id_ = rapidjson_get_string(iter_devid->value);

    auto iter_aimsid = _node.FindMember("aimsid");
    if (iter_aimsid != _node.MemberEnd() && iter_aimsid->value.IsString())
        aimsid_ = rapidjson_get_string(iter_aimsid->value);

    // TODO : time_t
    auto iter_expiredin = _node.FindMember("expiredin");
    if (iter_expiredin != _node.MemberEnd() && iter_expiredin->value.IsInt64())
    {
        exipired_in_ = iter_expiredin->value.GetInt64();
    }

    auto iter_timeoffset = _node.FindMember("timeoffset");
    if (iter_timeoffset != _node.MemberEnd() && iter_timeoffset->value.IsInt64())
    {
        time_offset_ = iter_timeoffset->value.GetInt64();
    }

    auto iter_login = _node.FindMember("login");
    if (iter_login != _node.MemberEnd() && iter_login->value.IsString())
    {
        login_ = rapidjson_get_string(iter_login->value);
    }

    auto iter_fetchurl = _node.FindMember("fetchurl");
    if (iter_fetchurl != _node.MemberEnd() && iter_fetchurl->value.IsString())
    {
        fetch_url_ = rapidjson_get_string(iter_fetchurl->value);
    }

    auto iter_password_md5 = _node.FindMember("password_md5");
    if (iter_password_md5 != _node.MemberEnd() && iter_password_md5->value.IsString())
    {
        password_md5_ = rapidjson_get_string(iter_password_md5->value);
    }

    return true;
}

void core::wim::auth_parameters::serialize(rapidjson::Value& _node, rapidjson_allocator& _a) const
{
    _node.AddMember("login", login_, _a);
    _node.AddMember("aimid", aimid_, _a);
    _node.AddMember("atoken", a_token_, _a);
    _node.AddMember("devid", dev_id_, _a);
    _node.AddMember("expiredin", (int64_t)exipired_in_, _a);
    _node.AddMember("timeoffset", (int64_t)time_offset_, _a);
    _node.AddMember("timeoffset_local", (int64_t)time_offset_local_, _a);
    _node.AddMember("aimsid", aimsid_, _a);
    _node.AddMember("fetchurl", fetch_url_, _a);
    _node.AddMember("password_md5", password_md5_, _a);
    _node.AddMember("productguid", product_guid_8x_, _a);
    _node.AddMember("agenttoken", agent_token_, _a);
    if (o2token_)
        _node.AddMember("o2token", *o2token_, _a);
}

bool core::wim::auth_parameters::unserialize(coll_helper& _params)
{
    aimid_ = _params.get_value_as_string("aimid");
    if (aimid_.empty())
        return false;

    a_token_ = _params.get_value_as_string("atoken");
    o2token_ = _params.get_value_as_string("o2token");
    agent_token_ = _params.get_value_as_string("agenttoken");
    product_guid_8x_ = _params.get_value_as_string("productguid");
    std::string dev_id = _params.get_value_as_string("devid");

    if (!dev_id.empty())
        dev_id_ = std::move(dev_id);

    aimsid_ = _params.get_value_as_string("aimsid");

    if (_params.is_value_exist("expiredin"))
        exipired_in_ = _params.get_value_as_int64("expiredin");

    time_offset_ = _params.get_value_as_int64("timeoffset");

    if (_params.is_value_exist("timeoffset_local"))
        time_offset_local_ = _params.get_value_as_int64("timeoffset_local");

    login_ = _params.get_value_as_string("login");
    fetch_url_ = _params.get_value_as_string("fetchurl");
    password_md5_ = _params.get_value_as_string("password_md5");

    return true;
}


fetch_parameters::fetch_parameters()
    :
    next_fetch_time_(std::chrono::system_clock::now()),
    next_fetch_timeout_(std::chrono::seconds(60)),
    last_successful_fetch_(0)
{
}

void fetch_parameters::serialize(core::tools::binary_stream& _stream) const
{
    core::tools::tlvpack pack;
    core::tools::binary_stream temp_stream;

    pack.push_child(core::tools::tlv(fpt_fetch_url, hide_url_param(fetch_url_, "aimsid", aimsid_single_range_evaluator())));
    pack.push_child(core::tools::tlv(fpt_last_successful_fetch, (int64_t) last_successful_fetch_));

    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool fetch_parameters::unserialize(core::tools::binary_stream& _stream, const std::string& _aimsid)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_fetch_url = tlv_pack_childs.get_item(fpt_fetch_url);
    auto tlv_last_successfull_fetch = tlv_pack_childs.get_item(fpt_last_successful_fetch);

    if (
        !tlv_fetch_url ||
        !tlv_last_successfull_fetch)
        return false;

    fetch_url_ = recover_hidden_url_param(tlv_fetch_url->get_value<std::string>(), "aimsid", _aimsid);
    last_successful_fetch_ = tlv_last_successfull_fetch->get_value<int64_t>(0);

    return true;
}

bool fetch_parameters::is_valid() const
{
    return !fetch_url_.empty();
}

std::string core::wim::hide_url_param(const std::string& _url, const std::string& _param, url_range_evaluator _range_eval)
{
    auto url = _url;
    auto param = _param + '=';
    if (auto begin_pos = url.find(param); begin_pos != std::string::npos)
    {
        begin_pos += param.size();
        auto end_pos = url.find('&', begin_pos);
        if (end_pos == std::string::npos)
            end_pos = url.size();

        size_t hide_len = end_pos - begin_pos;
        if (_range_eval)
        {
            auto [start_hide_pos, end_hide_pos] = _range_eval(std::string_view(url.c_str() + begin_pos, hide_len));
            begin_pos += start_hide_pos;
            hide_len = end_hide_pos - start_hide_pos + 1;
        }

        url.replace(begin_pos, hide_len, hide_len, '*');
    }

    return url;
}

std::string core::wim::recover_hidden_url_param(const std::string& _url, const std::string& _param, const std::string& _value)
{
    auto url = _url;
    auto param = _param + '=';
    if (auto begin = url.find(param); begin != std::string::npos)
    {
        begin += param.size();
        auto end = url.find('&', begin);
        if (end == std::string::npos)
            end = url.size();

        url.replace(url.begin() + begin, url.begin() + end, _value.begin(), _value.end());
    }

    return url;
}
