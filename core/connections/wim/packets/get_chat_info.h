#pragma once

#include "../robusto_packet.h"
#include "../chat_info.h"
#include "../persons_packet.h"

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
        struct get_chat_info_params
        {
            std::string aimid_;
            std::string stamp_;
            uint32_t members_limit_ = 0;
        };

        class get_chat_info: public robusto_packet, public persons_packet
        {
        public:
            get_chat_info(
                wim_packet_params _params,
                get_chat_info_params _chat_params);

            virtual ~get_chat_info();

            int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            int32_t parse_results(const rapidjson::Value& _node_results) override;
            int32_t on_response_error_code() override;

            const std::shared_ptr<core::archive::persons_map>& get_persons() const override { return result_.get_persons(); }

            const auto& get_result() const { return result_; }

            priority_t get_priority() const override { return priority_protocol(); }

            std::string_view get_method() const override;
            int minimal_supported_api_version() const override;

        private:
            get_chat_info_params params_;
            chat_info result_;
        };
    }
}