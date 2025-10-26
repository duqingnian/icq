#pragma once

#include "../robusto_packet.h"
#include "../chat_params.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace wim
    {
        class mod_chat : public robusto_packet
        {
        private:
            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_response_data(const rapidjson::Value& _data) override;

            std::string	aimid_;
            chat_params chat_params_;

        public:
            mod_chat(wim_packet_params _params, const std::string& _aimId);
            virtual ~mod_chat();

            chat_params& get_chat_params();
            void set_chat_params(chat_params _chat_params);
            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;
        };

    }

}
