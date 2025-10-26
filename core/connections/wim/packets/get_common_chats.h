#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }
}


namespace core
{
    namespace wim
    {
        class get_common_chats: public robusto_packet
        {
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            std::string sn_;

        public:
            std::vector<common_chat_info> result_;

            get_common_chats(wim_packet_params _params, const std::string& _sn);

            virtual ~get_common_chats();

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}