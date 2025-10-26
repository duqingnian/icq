#include "stdafx.h"
#include "chat_params.h"

using namespace core;
using namespace wim;

void chat_params::set_name(const std::string& _name)
{
    name_= _name;
}

void chat_params::set_avatar(const std::string& _avatar)
{
    avatar_ = _avatar;
}

void chat_params::set_about(const std::string& _about)
{
    about_ = _about;
}

void chat_params::set_rules(const std::string& _rules)
{
    rules_ = _rules;
}

void chat_params::set_stamp(const std::string& _stamp)
{
    stamp_ = _stamp;
}

void chat_params::set_public(bool _public)
{
    public_ = _public;
}

void chat_params::set_join(bool _approved)
{
    approved_ = _approved;
}

void chat_params::set_joiningByLink(bool _joiningByLink)
{
    joiningByLink_ = _joiningByLink;
}

void chat_params::set_readOnly(bool _readOnly)
{
    readOnly_ = _readOnly;
}

void core::wim::chat_params::set_isChannel(bool _isChannel)
{
    isChannel_ = _isChannel;
}

void chat_params::set_trust_required(bool _trust_required)
{
    trust_required_ = _trust_required;
}

void core::wim::chat_params::set_threads_enabled(bool _threads_enabled)
{
    threads_enabled_ = _threads_enabled;
}

void chat_params::set_threads_auto_subscribe(bool _threads_auto_subscribe)
{
    threads_auto_subscribe_ = _threads_auto_subscribe;
}

chat_params chat_params::create(const core::coll_helper& _params)
{
    auto result = chat_params();
    if (_params.is_value_exist("name"))
        result.set_name(_params.get_value_as_string("name"));
    if (_params.is_value_exist("avatar"))
        result.set_avatar(_params.get_value_as_string("avatar"));
    if (_params.is_value_exist("about"))
        result.set_about(_params.get_value_as_string("about"));
    if (_params.is_value_exist("public"))
        result.set_public(_params.get_value_as_bool("public"));
    if (_params.is_value_exist("approved"))
        result.set_join(_params.get_value_as_bool("approved"));
    if (_params.is_value_exist("link"))
        result.set_joiningByLink(_params.get_value_as_bool("link"));
    if (_params.is_value_exist("ro"))
        result.set_readOnly(_params.get_value_as_bool("ro"));
    if (_params.is_value_exist("stamp"))
        result.set_stamp(_params.get_value_as_string("stamp"));
    if (_params.is_value_exist("is_channel"))
        result.set_isChannel(_params.get_value_as_bool("is_channel"));
    if (_params.is_value_exist("trustRequired"))
        result.set_trust_required(_params.get_value_as_bool("trustRequired"));
    if (_params.is_value_exist("threadsEnabled"))
        result.set_threads_enabled(_params.get_value_as_bool("threadsEnabled"));
    if (_params.is_value_exist("threadsAutoSubscribe"))
        result.set_threads_auto_subscribe(_params.get_value_as_bool("threadsAutoSubscribe"));

    return result;
}
