#include "stdafx.h"

#include "../../../../corelib/enumerations.h"

#include "../../../gui_settings.h"

#include "FileSharingUtils.h"

#include "TextChunk.h"

#include "../../../url_config.h"

#include "utils/UrlParser.h"

const Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::Empty(Ui::ComplexMessage::TextChunk::Type::Undefined, QString(), QString(), -1);

namespace
{
    bool arePreviewsEnabled()
    {
        return Ui::get_gui_settings()->get_value<bool>(settings_show_video_and_images, true);
    }

    bool isSnippetUrl(QStringView _url)
    {
        static constexpr QStringView urls[] = { u"https://jira.mail.ru", u"jira.mail.ru", u"https://confluence.mail.ru", u"confluence.mail.ru", u"https://sys.mail.ru", u"sys.mail.ru" };
        return std::none_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
    }

    bool shouldForceSnippetUrl(QStringView _url)
    {
        static constexpr QStringView urls[] = { u"cloud.mail.ru", u"https://cloud.mail.ru" };
        return std::any_of(std::begin(urls), std::end(urls), [&_url](const auto& x) { return _url.startsWith(x); });
    }

    bool isAlwaysWithPreview(const Utils::UrlParser& _parser)
    {
        const bool isSticker = _parser.isFileSharing() &&
            Ui::ComplexMessage::getFileSharingContentType(_parser.rawUrlString()).is_sticker();
        return isSticker || _parser.isProfile() || _parser.isMedia();
    }
}


Ui::ComplexMessage::ChunkIterator::ChunkIterator(const QString& _text, Utils::UrlParser& _parser)
    : tokenizer_(std::u16string_view{ (const char16_t*)_text.utf16(), (size_t)_text.size() })
    , parser_(&_parser)
{
}

Ui::ComplexMessage::ChunkIterator::ChunkIterator(const Data::FString& _text, Utils::UrlParser& _parser)
    : tokenizer_(std::u16string_view{ (const char16_t*)_text.string().utf16(), (size_t)_text.size() }, _text.formatting())
    , formattedText_(_text)
    , parser_(&_parser)
{
}

bool Ui::ComplexMessage::ChunkIterator::hasNext() const
{
    return tokenizer_.has_token();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::ChunkIterator::current(bool _allowSnipet, bool _forcePreview) const
{
    const auto& token = tokenizer_.current();

    if (token.type_ == common::tools::message_token::type::text)
    {
        const auto& text = boost::get<common::tools::tokenizer_string>(token.data_);
        auto sourceText = QString::fromStdWString(text);
        return TextChunk(TextChunk::Type::Text, std::move(sourceText));
    }
    else if (token.type_ == common::tools::message_token::type::formatted_text)
    {
        const auto& text = boost::get<common::tools::tokenizer_string>(token.data_);
        auto sourceText = QString::fromStdWString(text);
        const auto textSize = sourceText.size();
        im_assert(token.formatted_source_offset_ >= 0);
        im_assert(token.formatted_source_offset_ < formattedText_.size());
        const auto view = formattedText_.mid(token.formatted_source_offset_, textSize);
        const auto type = view.containsFormat(core::data::format_type::pre) ? TextChunk::Type::Pre : TextChunk::Type::FormattedText;
        return TextChunk(view, type);
    }

    im_assert(token.type_ == common::tools::message_token::type::url);

    const auto& url = boost::get<common::tools::url_view>(token.data_);

    auto linkText = QString::fromStdWString(url.text_);
    im_assert(!linkText.isEmpty());
    auto linkTextView = decltype(formattedText_)();

    parser_->setUrl(url.scheme_, url.text_);

    const auto forbidPreview = parser_->isSite() && !arePreviewsEnabled();
    if (!formattedText_.isEmpty() && token.formatted_source_offset_ != -1)
    {
        const auto textSize = linkText.size();
        im_assert(token.formatted_source_offset_ >= 0);
        im_assert(token.formatted_source_offset_ + textSize <= formattedText_.size());
        linkTextView = formattedText_.mid(token.formatted_source_offset_, textSize);
        im_assert(!linkTextView.isEmpty());
    }

    auto addChunk = [&linkText, linkTextView](TextChunk::Type _plainChunkType, TextChunk::Type _formattedChunkType, QString _imageType = {})
    {
        return linkTextView.isEmpty()
            ? TextChunk(_plainChunkType, std::move(linkText), _imageType)
            : TextChunk(linkTextView, _formattedChunkType, _imageType);
    };

    if (!parser_->isFileSharing() &&
        !parser_->isImage() &&
        !parser_->isEmail() &&
        !parser_->isProfile() &&
        !_allowSnipet || !isSnippetUrl(linkText) ||
        (forbidPreview && !_forcePreview && !isAlwaysWithPreview(*parser_)))
    {
        return addChunk(TextChunk::Type::Text, TextChunk::Type::FormattedText);
    }

    im_assert(!linkTextView.isEmpty());
    switch (parser_->category())
    {
    case Utils::UrlParser::UrlCategory::Image:
    case Utils::UrlParser::UrlCategory::Video:
    {
        // spike for cloud.mail.ru, (temporary code)
        if (shouldForceSnippetUrl(linkText))
            return addChunk(TextChunk::Type::GenericLink, TextChunk::Type::GenericLink);

        return addChunk(TextChunk::Type::ImageLink, TextChunk::Type::ImageLink, parser_->fileSuffix());
    }
    case Utils::UrlParser::UrlCategory::FileSharing:
    {
        const auto id = extractIdFromFileSharingUri(linkText);
        const auto content_type = extractContentTypeFromFileSharingId(id.fileId_);

        auto Type = TextChunk::Type::FileSharingGeneral;
        switch (content_type.type_)
        {
        case core::file_sharing_base_content_type::image:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingImageSticker : TextChunk::Type::FileSharingImage;
            break;
        case core::file_sharing_base_content_type::gif:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingGifSticker : TextChunk::Type::FileSharingGif;
            break;
        case core::file_sharing_base_content_type::video:
            Type = content_type.is_sticker() ? TextChunk::Type::FileSharingVideoSticker : TextChunk::Type::FileSharingVideo;
            break;
        case core::file_sharing_base_content_type::ptt:
            Type = TextChunk::Type::FileSharingPtt;
            break;
        case core::file_sharing_base_content_type::lottie:
            Type = TextChunk::Type::FileSharingLottieSticker;
            break;
        default:
            break;
        }

        const auto durationSec = extractDurationFromFileSharingId(id.fileId_);
        return TextChunk(Type, std::move(linkText), QString(), durationSec);
    }
    case Utils::UrlParser::UrlCategory::Site:
    {
        if (parser_->hasScheme())
            return addChunk(TextChunk::Type::GenericLink, TextChunk::Type::GenericLink);
        else
            return addChunk(TextChunk::Type::Text, TextChunk::Type::FormattedText);
    }
    case Utils::UrlParser::UrlCategory::Email:
        return addChunk(TextChunk::Type::Text, TextChunk::Type::FormattedText);
    case Utils::UrlParser::UrlCategory::Ftp:
        return addChunk(TextChunk::Type::GenericLink, TextChunk::Type::FormattedText);
    case Utils::UrlParser::UrlCategory::ImProtocol:
        return addChunk(TextChunk::Type::GenericLink, TextChunk::Type::FormattedText);
    case Utils::UrlParser::UrlCategory::Profile:
        return TextChunk(TextChunk::Type::ProfileLink, std::move(linkText));
    default:
        break;
    }

    im_assert(!"invalid url type");
    return TextChunk(TextChunk::Type::Text, std::move(linkText));
}

void Ui::ComplexMessage::ChunkIterator::next()
{
    tokenizer_.next();
}

Ui::ComplexMessage::TextChunk::TextChunk()
    : Ui::ComplexMessage::TextChunk(Ui::ComplexMessage::TextChunk::Empty)
{
}

Ui::ComplexMessage::TextChunk::TextChunk(Type _type, QString _text, QString _imageType, int32_t _durationSec)
    : Type_(_type)
    , text_(std::move(_text))
    , ImageType_(std::move(_imageType))
    , DurationSec_(_durationSec)
{
    im_assert(Type_ > Type::Min);
    im_assert(Type_ < Type::Max);
    im_assert((Type_ != Type::ImageLink) || !ImageType_.isEmpty());
    im_assert(DurationSec_ >= -1);
}

Ui::ComplexMessage::TextChunk::TextChunk(Data::FStringView _view, Type _type, QString _imageType)
    : TextChunk(_type, _view.string().toString(), _imageType)
{
    formattedText_ = _view;
}

int32_t Ui::ComplexMessage::TextChunk::length() const
{
    return getPlainText().length();
}

Ui::ComplexMessage::TextChunk Ui::ComplexMessage::TextChunk::mergeWith(const TextChunk& _other) const
{
    if ((Type_ == Type::FormattedText && _other.Type_ == Type::GenericLink) ||
        (Type_ == Type::GenericLink && _other.Type_ == Type::FormattedText) ||
        (Type_ == Type::FormattedText && _other.Type_ == Type::FormattedText))
    {
        if (auto text = getFView(); text.tryToAppend(_other.getFView()))
            return text;
        return TextChunk::Empty;
    }
    else if (Type_ != Type::Text && Type_ != Type::GenericLink)
    {
        return TextChunk::Empty;
    }
    else if (_other.Type_ != Type::Text && _other.Type_ != Type::GenericLink)
    {
        return TextChunk::Empty;
    }
    else
    {
        auto textResult = getPlainText().toString();
        textResult += _other.getPlainText().toString();
        return TextChunk(Type::Text, std::move(textResult));
    }
}
