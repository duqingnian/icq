#include "stdafx.h"

#include "../wim_packet.h"

#include "preview_proxy.h"

#include "../../../../common.shared/json_helper.h"
#include "../../urls_cache.h"
#include "../common.shared/string_utils.h"

CORE_WIM_PREVIEW_PROXY_NS_BEGIN

namespace
{
    int32_t favicon_size_2_px(const favicon_size _size);

    std::string parse_annotation(const rapidjson::Value &_doc_node);

    std::string parse_content_type(const rapidjson::Value &_doc_node);

    std::string parse_favicon(const rapidjson::Value &_doc_node);

    std::string parse_image_uri(const rapidjson::Value &_doc_node,
        Out preview_size &_size,
        Out std::string &_download_uri,
        Out int64_t &_file_size,
        Out origin_size &_origin_size);

    std::string parse_title(const rapidjson::Value &_doc_node);

    std::string parse_filename(const rapidjson::Value &_doc_node);
    std::string parse_fileformat(const rapidjson::Value& _doc_node);
}

link_meta::link_meta(const std::string &_title,
    const std::string &_annotation,
    const std::string &_preview_uri,
    const std::string &_download_uri,
    const std::string &_favicon_uri,
    const std::string &_content_type,
    const preview_size &_preview_size,
    const int64_t _file_size,
    const origin_size& _origin_size,
    const std::string& _filename,
    const std::string& _fileformat)
    : preview_uri_(_preview_uri)
    , download_uri_(_download_uri)
    , title_(_title)
    , annotation_(_annotation)
    , favicon_uri_(_favicon_uri)
    , content_type_(_content_type)
    , preview_size_(_preview_size)
    , file_size_(_file_size)
    , origin_size_(_origin_size)
    , filename_(_filename)
    , fileformat_(_fileformat)
    , status_code_(0)
{
    im_assert(!content_type_.empty());
    im_assert(std::get<0>(preview_size_) >= 0);
    im_assert(std::get<1>(preview_size_) >= 0);
    im_assert(std::get<0>(origin_size_) >= 0);
    im_assert(std::get<1>(origin_size_) >= 0);
    im_assert(file_size_ >= -1);
}

link_meta::link_meta(int64_t _status_code)
    : file_size_(0)
    , status_code_(_status_code)
{
}

link_meta::~link_meta()
{
}

std::string link_meta::get_preview_uri(
    const int32_t_opt _width,
    const int32_t_opt _height) const
{
    im_assert(!_width || (*_width >= 0));
    im_assert(!_height || (*_height >= 0));

    std::string result;

    constexpr auto MAX_ARGS_LENGTH = 20u;
    result.reserve(preview_uri_.size() + MAX_ARGS_LENGTH);

    result += preview_uri_;

    if (_width && (*_width > 0))
    {
        result += "&w=";
        result += std::to_string(*_width);
    }

    if (_height && (*_height > 0))
    {
        result += "&h=";
        result += std::to_string(*_height);
    }

    return result;
}

const std::string& link_meta::get_annotation() const
{
    return annotation_;
}

const std::string& link_meta::get_content_type() const
{
    im_assert(!content_type_.empty());

    return content_type_;
}

const std::string& link_meta::get_download_uri() const
{
    return download_uri_;
}

const std::string& link_meta::get_favicon_uri() const
{
    return favicon_uri_;
}

int64_t link_meta::get_file_size() const
{
    im_assert(file_size_ >= -1);
    return file_size_;
}

preview_size link_meta::get_preview_size() const
{
    return preview_size_;
}

origin_size link_meta::get_origin_size() const
{
    return origin_size_;
}

const std::string& link_meta::get_title() const
{
    return title_;
}

const std::string& link_meta::get_filename() const
{
    return filename_;
}

const std::string& link_meta::get_fileformat() const
{
    return fileformat_;
}

bool link_meta::has_favicon_uri() const
{
    return !favicon_uri_.empty();
}

bool link_meta::has_preview_uri() const
{
    return !preview_uri_.empty();
}

int64_t link_meta::get_status_code() const
{
    return status_code_;
}

str_2_str_map format_get_preview_params(
    std::string_view _uri_to_preview,
    const int32_t_opt _width,
    const int32_t_opt _height,
    const bool _crop,
    const favicon_size _favicon_size)
{
    im_assert(!_width || (*_width >= 0));
    im_assert(!_height || (*_height >= 0));
    im_assert(_favicon_size > favicon_size::min);
    im_assert(_favicon_size < favicon_size::max);

    str_2_str_map result;

    result.emplace("url", wim_packet::escape_symbols(_uri_to_preview));

    if (_width && (*_width > 0))
    {
        result.emplace("w", std::to_string(*_width));
    }

    if (_height && (*_height > 0))
    {
        result.emplace("h", std::to_string(*_height));
    }

    if (_crop)
    {
        im_assert(_width || _height);
        result.emplace("crop", "1");
    }

    const auto favicon_size_px = favicon_size_2_px(_favicon_size);
    im_assert(favicon_size_px > 0);

    if (const auto is_favicon_enabled = favicon_size_px > 0; is_favicon_enabled)
    {
        result.emplace("favicon", "1");
        result.emplace("favsize", std::to_string(favicon_size_px));
    }

    return result;
}

str_2_str_map format_get_url_content_params(std::string_view _uri)
{
    return str_2_str_map{ {"url", wim_packet::escape_symbols(_uri)} };
}

link_meta_uptr parse_json(InOut char *_json)
{
    im_assert(_json);

    rapidjson::Document doc;
    if (doc.ParseInsitu(_json).HasParseError())
    {
        return nullptr;
    }

    auto iter_status = doc.FindMember("status");
    if (iter_status == doc.MemberEnd() || !iter_status->value.IsString())
        return nullptr;

    if (!boost::iequals(iter_status->value.GetString(), "ok"))
    {
        auto iter_status_code = doc.FindMember("status_code");
        if (iter_status_code != doc.MemberEnd() && iter_status_code->value.IsString())
            return std::make_unique<link_meta>(std::atoi(iter_status_code->value.GetString()));
    }

    auto iter_doc = doc.FindMember("doc");
    if (iter_doc == doc.MemberEnd() || !iter_doc->value.IsObject())
        return nullptr;

    const auto &root_node = iter_doc->value;

    const auto annotation = parse_annotation(root_node);

    preview_size size(0, 0);
    origin_size o_size(0, 0);
    std::string download_uri;
    int64_t file_size = -1;

    const auto preview_uri = parse_image_uri(root_node, Out size, Out download_uri, Out file_size, Out o_size);

    const auto title = parse_title(root_node);

    const auto favicon_uri = parse_favicon(root_node);

    const auto content_type = parse_content_type(root_node);

    if (content_type.empty())
        return nullptr;

    const auto filename = parse_filename(root_node);

    const auto fileformat = parse_fileformat(root_node);

    return std::make_unique<link_meta>(
        title,
        annotation,
        preview_uri,
        download_uri,
        favicon_uri,
        content_type,
        size,
        file_size,
        o_size,
        filename,
        fileformat);
}

namespace uri
{
    std::string get_preview()
    {
        return su::concat(urls::get_url(urls::url_type::preview_host), "/getPreview");
    }

    std::string get_url_content()
    {
        return urls::get_url(urls::url_type::url_content);
    }
}

namespace
{
    int32_t favicon_size_2_px(const favicon_size _size)
    {
        im_assert(_size > favicon_size::min);
        im_assert(_size < favicon_size::max);

        switch(_size)
        {
            case favicon_size::small: return 16;
            case favicon_size::med: return 32;
            default:
                im_assert(!"unknown favicon size");
                return -1;
        }
    }

    std::string parse_annotation(const rapidjson::Value &_doc_node)
    {
        auto iter_annotation = _doc_node.FindMember("snippet");
        if ((iter_annotation == _doc_node.MemberEnd()) ||
            !iter_annotation->value.IsString())
        {
            return std::string();
        }

        return rapidjson_get_string(iter_annotation->value);
    }

    std::string parse_content_type(const rapidjson::Value &_doc_node)
    {
        std::string res;
        tools::unserialize_value(_doc_node, "content_type", res);
        return res;
    }

    std::string parse_favicon(const rapidjson::Value &_doc_node)
    {
        auto iter_favicon = _doc_node.FindMember("favicon");
        if ((iter_favicon == _doc_node.MemberEnd()) ||
            !iter_favicon->value.IsArray() ||
            iter_favicon->value.Empty())
        {
            return std::string();
        }

        const auto &node_image = iter_favicon->value[0];
        if (!node_image.IsObject() ||
            !node_image.HasMember("preview_url"))
        {
            return std::string();
        }

        const auto &node_preview_url = node_image["preview_url"];
        if (!node_preview_url.IsString())
        {
            return std::string();
        }

        return rapidjson_get_string(node_preview_url);
    }

    std::string parse_image_uri(const rapidjson::Value &_doc_node,
        Out preview_size &_size,
        Out std::string &_download_uri,
        Out int64_t &_file_size,
        Out origin_size &_origin_size)
    {
        Out _size = preview_size(0, 0);
        Out _download_uri = std::string();
        Out _file_size = -1;

        auto iter_images = _doc_node.FindMember("images");

        if ((iter_images == _doc_node.MemberEnd()) ||
            !iter_images->value.IsArray() ||
            iter_images->value.Empty())
        {
            return std::string();
        }

        const auto &node_image = iter_images->value[0];
        if (!node_image.IsObject() ||
            !node_image.HasMember("preview_url"))
        {
            return std::string();
        }

        const auto is_video = parse_content_type(_doc_node) == "video";


        if (const auto iter_custom = _doc_node.FindMember("custom_fields"); iter_custom != _doc_node.MemberEnd() && iter_custom->value.IsArray() && !iter_custom->value.Empty())
        {
            for (const auto& node : iter_custom->value.GetArray())
            {
                if (!node.IsObject())
                    continue;

                std::string_view name;
                if (!tools::unserialize_value(node, "name", name))
                    continue;

                if (name == "download_url")
                {
                    tools::unserialize_value(node, "value", _download_uri);
                    break;
                }
            }
        }
        else if (is_video)
        {
            tools::unserialize_value(_doc_node, "url", _download_uri);
        }
        else
        {
            const auto &node_url = node_image["url"];
            if (node_url.IsString())
            {
                Out _download_uri = rapidjson_get_string(node_url);
            }
        }

        const auto &node_preview_url = node_image["preview_url"];
        if (!node_preview_url.IsString())
        {
            return std::string();
        }

        auto preview_width = 0;

        const auto &node_preview_width = node_image["preview_width"];
        if (node_preview_width.IsString())
        {
            preview_width = std::stoi(rapidjson_get_string(node_preview_width));
        }

        auto preview_height = 0;

        const auto &node_preview_height = node_image["preview_height"];
        if (node_preview_height.IsString())
        {
            preview_height = std::stoi(rapidjson_get_string(node_preview_height));
        }

        const auto is_size_valid = ((preview_width > 0) && (preview_height > 0));
        if (is_size_valid)
        {
            Out _size = preview_size(preview_width, preview_height);
        }

        auto origin_width = 0;

        const auto &node_origin_width = node_image["origin_width"];
        if (node_origin_width.IsString())
        {
            origin_width = std::stoi(rapidjson_get_string(node_origin_width));
        }

        auto origin_height = 0;

        const auto &node_origin_height = node_image["origin_height"];
        if (node_origin_height.IsString())
        {
            origin_height = std::stoi(rapidjson_get_string(node_origin_height));
        }

        const auto is_size_origin_valid = ((origin_width > 0) && (origin_height > 0));
        if (is_size_origin_valid)
        {
            Out _origin_size = origin_size(origin_width, origin_height);
        }

        const auto &node_orig_size = node_image["orig_size"];
        if (node_orig_size.IsString() && !is_video)
        {
            int64_t orig_size = -1;

            try
            {
                orig_size = std::stoll(rapidjson_get_string(node_orig_size));
            }
            catch (std::invalid_argument&)
            {
                im_assert(!"invalid orig size string");
            }
            catch (std::out_of_range&)
            {
                im_assert(!"invalid orig size value");
            }

            const auto is_orig_size_valid = (orig_size > 0);
            if (is_orig_size_valid)
            {
                Out _file_size = orig_size;
            }
        }

        return rapidjson_get_string(node_preview_url);
    }

    std::string parse_title(const rapidjson::Value &_doc_node)
    {
        std::string res;
        tools::unserialize_value(_doc_node, "title", res);
        return res;
    }

    std::string parse_filename(const rapidjson::Value &_doc_node)
    {
        std::string res;
        tools::unserialize_value(_doc_node, "file_name", res);
        return res;
    }

    std::string parse_fileformat(const rapidjson::Value& _doc_node)
    {
        std::string res;
        auto iter_images = _doc_node.FindMember("images");

        if ((iter_images == _doc_node.MemberEnd()) ||
            !iter_images->value.IsArray() ||
            iter_images->value.Empty())
        {
            return res;
        }

        tools::unserialize_value(iter_images->value[0], "format", res);
        return res;
    }
}

CORE_WIM_PREVIEW_PROXY_NS_END
