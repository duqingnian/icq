#pragma once

#include "../wim_packet.h"
#include "smartreply/smartreply_marker.h"

namespace core
{
    namespace tools
    {
        class http_request_simple;
    }

    namespace archive
    {
        class quote;
        struct shared_contact_data;
        struct geo_data;
        using quotes_vec = std::vector<quote>;
        using mentions_map = std::map<std::string, std::string>;
        using shared_contact = std::optional<shared_contact_data>;
        using geo = std::optional<geo_data>;
        using poll = std::optional<poll_data>;
    } // namespace archive
} // namespace core

namespace core
{
    enum class message_type;

    namespace wim
    {
        class send_message : public wim_packet
        {
            virtual int32_t init_request(const std::shared_ptr<core::http_request_simple>& _request) override;
            virtual int32_t parse_response_data(const rapidjson::Value& _data) override;
            virtual int32_t execute_request(const std::shared_ptr<core::http_request_simple>& _request) override;

            const int64_t updated_id_;
            const message_type type_;

            std::string aimid_;
            std::string message_text_;
            core::data::format message_format_;

            uint32_t sms_error_;
            uint32_t sms_count_;

            std::string internal_id_;

            int64_t hist_msg_id_;
            int64_t before_hist_msg_id;
            std::string poll_id_;
            std::string task_id_;

            bool duplicate_;

            core::archive::quotes_vec quotes_;
            core::archive::mentions_map mentions_;

            std::string description_;
            core::data::format description_format_;
            std::string url_;

            core::archive::shared_contact shared_contact_;
            core::archive::geo geo_;
            core::archive::poll poll_;
            core::tasks::task task_;
            smartreply::marker_opt smartreply_marker_;

            std::optional<int64_t> draft_delete_time_;

        public:
            uint32_t get_sms_error() const { return sms_error_; }
            uint32_t get_sms_count() const { return sms_count_; }

            const std::string& get_internal_id() const { return internal_id_; }

            int64_t get_hist_msg_id() const { return hist_msg_id_; }
            int64_t get_before_hist_msg_id() const { return before_hist_msg_id; }

            int64_t get_updated_id() const { return updated_id_; }

            bool is_duplicate() const { return duplicate_; }

            const std::string& get_poll_id() const { return poll_id_; }
            const std::string& get_task_id() const { return task_id_; }

            send_message(
                wim_packet_params _params,
                const int64_t _updated_id,
                const message_type _type,
                const std::string& _internal_id,
                const std::string& _aimid,
                const std::string& _message_text,
                const core::data::format& _message_format,
                const core::archive::quotes_vec& _quotes,
                const core::archive::mentions_map& _mentions,
                const std::string& _description,
                const core::data::format& _description_format,
                const std::string& _url,
                const core::archive::shared_contact& _shared_contact,
                const core::archive::geo& _geo,
                const core::archive::poll& _poll,
                const core::tasks::task& _task,
                const smartreply::marker_opt& _smartreply_marker,
                const std::optional<int64_t>& _draft_delete_time);

            virtual ~send_message();

            priority_t get_priority() const override;
            bool is_post() const override { return true; }

            std::string_view get_method() const override;

            bool support_self_resending() const override { return true; }
            bool support_partially_async_execution() const override { return true; }

            int minimal_supported_api_version() const override;
        };

    } // namespace wim

} // namespace core
