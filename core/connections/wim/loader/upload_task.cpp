#include "stdafx.h"

#include "loader_handlers.h"
#include "web_file_info.h"
#include "../packets/get_gateway.h"
#include "../packets/send_file.h"
#include "../../../tools/system.h"
#include "loader.h"

#include "../../../../common.shared/loader_errors.h"

#include "upload_task.h"
#include "loader.h"

using namespace core;
using namespace wim;

constexpr int32_t status_code_too_large_file = 413;
constexpr int32_t max_block_size = 1024 * 1024;

upload_task::upload_task(const std::string &_id, const wim_packet_params& _params, upload_file_params&& _file_params)
    : fs_loader_task(_id, _params)
    , file_params_(std::make_unique<upload_file_params>(std::move(_file_params)))
    , file_size_(0)
    , bytes_sent_(0)
    , start_time_(std::chrono::system_clock::now())
{
    session_id_ = std::chrono::system_clock::to_time_t(start_time_);
}


upload_task::~upload_task()
{
    if (file_stream_.is_open())
        file_stream_.close();
}

loader_errors upload_task::get_gate()
{
    get_gateway packet(get_wim_params(), core::tools::from_utf16(file_name_short_), file_size_, file_params_->duration, file_params_->base_content_type, file_params_->locale);

    int32_t res = packet.execute();

    if (res != 0)
    {
        if (wim_packet::is_network_error(res))
            return loader_errors::network_error;
        else if (res == wim_protocol_internal_error::wpie_error_too_large_file)
            return loader_errors::too_large_file;

        return loader_errors::get_gateway_error;
    }

    if (packet.get_status_code() != 200)
    {
        if (packet.get_status_code() == status_code_too_large_file)
            return loader_errors::max_file_size_error;

        return loader_errors::get_gateway_error;
    }

    upload_host_ = packet.get_host();
    upload_url_ = packet.get_url();

    return loader_errors::success;
}

loader_errors upload_task::open_file()
{
    boost::filesystem::wpath path_for_file(file_params_->file_name);
    if (!core::tools::system::is_exist(file_params_->file_name))
        return loader_errors::file_not_exits;

    file_name_short_ = path_for_file.filename().wstring();

#ifdef _WIN32
    file_stream_.open(file_params_->file_name, std::ifstream::binary | std::ifstream::in | std::ifstream::ate);
#else
    file_stream_.open(tools::from_utf16(file_params_->file_name), std::ifstream::binary | std::ifstream::in | std::ifstream::ate);
#endif
    if (!file_stream_.is_open())
        return loader_errors::file_not_exits;

    file_size_ = file_stream_.tellg();
    if (file_size_ == 0)
        return loader_errors::empty_file;

    file_stream_.seekg (0, std::ifstream::beg);

    out_buffer_.reserve(max_block_size);

    return loader_errors::success;
}

loader_errors upload_task::read_data_from_file()
{
    out_buffer_.reset();

    int64_t tail = file_size_ - bytes_sent_;
    int64_t read_size = max_block_size;

    if (tail <= 0)
    {
        im_assert(false);
        return loader_errors::internal_logic_error;
    }

    if (tail <= max_block_size)
        read_size = tail;

    file_stream_.seekg(bytes_sent_, std::ifstream::beg);

    file_stream_.read(out_buffer_.alloc_buffer((uint32_t)read_size), read_size);
    if (!file_stream_.good())
        return loader_errors::read_from_file;

    return loader_errors::success;
}

loader_errors upload_task::send_data_to_server()
{
    send_file_params chunk;
    chunk.size_already_sent_ = bytes_sent_;
    chunk.current_chunk_size_ = out_buffer_.available();
    chunk.full_data_size_ = file_size_;
    chunk.file_name_ = core::tools::from_utf16(file_name_short_);
    chunk.data_ = out_buffer_.read(out_buffer_.available());
    chunk.session_id_ = session_id_;

    send_file packet(get_wim_params(), chunk, upload_host_, upload_url_);

    uint32_t res = packet.execute();
    if (res != 0)
    {
        if (wim_packet::is_network_error(res))
            return loader_errors::network_error;

        return loader_errors::send_range;
    }

    bytes_sent_ += chunk.current_chunk_size_;

    if (packet.get_status_code() == 200)
    {
        file_url_ = packet.get_file_url();
        file_id_ = packet.get_file_id();
        im_assert(bytes_sent_ == file_size_);
    }
    else
    {
        im_assert(bytes_sent_ < file_size_);
    }

    return loader_errors::success;
}

loader_errors upload_task::send_next_range()
{
    auto res = read_data_from_file();
    if (res != loader_errors::success)
        return res;

    return send_data_to_server();
}

bool upload_task::is_end() const
{
    im_assert(bytes_sent_ <= file_size_);

    return (bytes_sent_ == file_size_);
}

const std::string& upload_task::get_file_url() const
{
    return file_url_;
}

const std::string& upload_task::get_file_id() const
{
    return file_id_;
}

void upload_task::set_handler(std::shared_ptr<upload_progress_handler> _handler)
{
    handler_ = std::move(_handler);
}

const std::shared_ptr<upload_progress_handler>& upload_task::get_handler() const
{
    return handler_;
}

void upload_task::on_result(int32_t _error)
{
    if (!get_handler()->on_result)
        return;

    get_handler()->on_result(_error, *make_info());
}

std::shared_ptr<web_file_info> upload_task::make_info() const
{
    auto info = std::make_shared<web_file_info>();

    info->set_file_name(file_params_->file_name);
    info->set_file_name_short(core::tools::from_utf16(file_name_short_));
    info->set_bytes_transfer(bytes_sent_);
    info->set_file_size(file_size_);
    info->set_file_url(file_url_);
    info->set_file_id(file_id_);

    return info;
}

void upload_task::on_progress()
{
    if (get_handler()->on_progress)
        get_handler()->on_progress(*make_info());
}

void upload_task::resume(loader& _loader)
{
    _loader.send_task_ranges_async(weak_from_this());
}

std::chrono::time_point<std::chrono::system_clock> upload_task::get_start_time() const
{
    return start_time_;
}

int64_t upload_task::get_file_size() const
{
    return file_size_;
}
