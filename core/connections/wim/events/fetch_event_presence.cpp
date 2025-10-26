#include "stdafx.h"

#include "fetch_event_presence.h"
#include "../wim_im.h"
#include "../wim_packet.h"
#include "../wim_contactlist_cache.h"
#include "../../../../common.shared/json_helper.h"

using namespace core;
using namespace wim;

fetch_event_presence::fetch_event_presence()
    :    presense_(std::make_shared<cl_presence>())
{
}


fetch_event_presence::~fetch_event_presence()
{
}

int32_t fetch_event_presence::parse(const rapidjson::Value& _node_event_data)
{
    if (!tools::unserialize_value(_node_event_data, "aimId", aimid_))
        return wpie_error_parse_response;

    presense_->unserialize(_node_event_data);
    return 0;
}

void fetch_event_presence::on_im(std::shared_ptr<core::wim::im> _im, std::shared_ptr<auto_callback> _on_complete)
{
    _im->on_event_presence(this, _on_complete);
}
