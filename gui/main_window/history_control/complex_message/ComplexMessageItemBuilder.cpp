﻿#include "stdafx.h"

#include "../common.shared/config/config.h"
#include "../common.shared/string_utils.h"
#include "../../../../corelib/enumerations.h"

#include "../../../utils/utils.h"
#include "../../../utils/UrlParser.h"
#include "../../../utils/features.h"

#include "../FileSharingInfo.h"

#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"
#include "FileSharingUtils.h"
#include "PttBlock.h"
#include "TextBlock.h"
#include "DebugTextBlock.h"
#include "QuoteBlock.h"
#include "StickerBlock.h"
#include "TextChunk.h"
#include "ProfileBlock.h"
#include "PollBlock.h"
#include "TaskBlock.h"
#include "CodeBlock.h"
#include "SnippetBlock.h"

#include "ComplexMessageItemBuilder.h"
#include "../../../controls/TextUnit.h"
#include "../../../controls/textrendering/TextRendering.h"
#include "../../../app_config.h"
#include "../../../gui_settings.h"
#include "memory_stats/MessageItemMemMonitor.h"
#include "main_window/contact_list/RecentsModel.h"
#include "url_config.h"

namespace
{
    bool isMediaType(Ui::ComplexMessage::TextChunk::Type _type)
    {
        using Type = Ui::ComplexMessage::TextChunk::Type;
        constexpr auto types = std::array
        {
            Type::ImageLink,
            Type::FileSharingImage,
            Type::FileSharingImageSticker,
            Type::FileSharingGif,
            Type::FileSharingGifSticker,
            Type::FileSharingVideo,
            Type::FileSharingVideoSticker
        };
        return std::any_of(types.begin(), types.end(), [_type](Type _mediaType) { return _type == _mediaType; });
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

core::file_sharing_content_type getFileSharingContentType(const TextChunk::Type _type)
{
    switch (_type)
    {
    case TextChunk::Type::FileSharingImage:
        return core::file_sharing_base_content_type::image;

    case TextChunk::Type::FileSharingImageSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::image, core::file_sharing_sub_content_type::sticker);

    case TextChunk::Type::FileSharingGif:
        return core::file_sharing_base_content_type::gif;

    case TextChunk::Type::FileSharingGifSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::gif, core::file_sharing_sub_content_type::sticker);

    case TextChunk::Type::FileSharingVideo:
        return core::file_sharing_base_content_type::video;

    case TextChunk::Type::FileSharingVideoSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::video, core::file_sharing_sub_content_type::sticker);

    case TextChunk::Type::FileSharingLottieSticker:
        return core::file_sharing_content_type(core::file_sharing_base_content_type::lottie, core::file_sharing_sub_content_type::sticker);

    default:
        break;
    }

    return core::file_sharing_content_type();
}

void registerFixedUrls(Utils::UrlParser& _parser)
{
    const QString profileDomainCommon = Features::getProfileDomain();
    const QString profileDomainAgent = Features::getProfileDomainAgent();

    _parser.registerUrlPattern(Features::getProfileDomain() % ql1c('/'), Utils::UrlParser::UrlCategory::Profile);
    _parser.registerUrlPattern(Features::getProfileDomainAgent() % ql1c('/'), Utils::UrlParser::UrlCategory::Profile);
    const auto& extraUrls = Ui::getUrlConfig().getFsUrls();
    for (const auto& url : extraUrls)
        _parser.registerUrlPattern(url % ql1c('/'));
}

QString convertPollQuoteText(const QString& _text) // replace media with placeholders
{
    QString result;

    Utils::UrlParser parser;
    registerFixedUrls(parser);

    ChunkIterator it(_text, parser);
    while (it.hasNext())
    {
        auto chunk = it.current(true, false);

        switch (chunk.Type_)
        {
            case TextChunk::Type::FileSharingImage:
                result += QT_TRANSLATE_NOOP("poll_block", "Photo");
                break;
            case TextChunk::Type::FileSharingGif:
                result += QT_TRANSLATE_NOOP("poll_block", "GIF");
                break;
            case TextChunk::Type::FileSharingGifSticker:
            case TextChunk::Type::FileSharingImageSticker:
            case TextChunk::Type::FileSharingVideoSticker:
            case TextChunk::Type::FileSharingLottieSticker:
                result += QT_TRANSLATE_NOOP("poll_block", "Sticker");
                break;
            case TextChunk::Type::FileSharingVideo:
                result += QT_TRANSLATE_NOOP("poll_block", "Video");
                break;
            case TextChunk::Type::FileSharingPtt:
                result += QT_TRANSLATE_NOOP("poll_block", "Voice message");
                break;
            case TextChunk::Type::FileSharingUpload:
            case TextChunk::Type::FileSharingGeneral:
                result += QT_TRANSLATE_NOOP("poll_block", "File");
                break;
            case TextChunk::Type::Text:
            case TextChunk::Type::GenericLink:
            case TextChunk::Type::ProfileLink:
            case TextChunk::Type::ImageLink:
            case TextChunk::Type::Junk:
                if (chunk.getPlainText().startsWith(u'\n'))
                    result += QChar::LineFeed;
                else if(chunk.getPlainText().startsWith(u' '))
                    result += QChar::Space;

                result += chunk.getPlainText().trimmed().toString();
                break;

            default:
                im_assert(!"unknown chunk type");
                result += chunk.getPlainText().toString();
                break;
        }

        if (chunk.getPlainText().endsWith(u'\n'))
            result += QChar::LineFeed;

        it.next();
    }

    return result;
}

SnippetBlock::EstimatedType estimatedTypeFromExtension(QStringView _extension)
{
    if (Utils::is_video_extension(_extension))
        return SnippetBlock::EstimatedType::Video;
    else if (Utils::is_image_extension_not_gif(_extension))
        return SnippetBlock::EstimatedType::Image;
    else if (Utils::is_image_extension(_extension))
        return SnippetBlock::EstimatedType::GIF;

    return SnippetBlock::EstimatedType::Article;
}

struct ParseResult
{
    std::list<TextChunk> chunks;
    Data::FString snippetUrl_;
    QString sourceText_;
    int snippetsCount = 0;
    int mediaCount = 0;
    bool linkInsideText = false;
    bool hasTrailingLink = false;
    Data::SharedContact sharedContact_;
    Data::Geo geo_;
    Data::Poll poll_;
    Data::Task task_;

    void mergeChunks()
    {
        for (auto iter = chunks.begin(); iter != chunks.end(); )
        {
            const auto next = std::next(iter);

            if (next == chunks.end())
                break;

            auto merged = iter->mergeWith(*next);
            if (merged.Type_ != TextChunk::Type::Undefined)
            {
                chunks.erase(iter);
                *next = std::move(merged);
            }

            iter = next;
        }
    }
};

std::vector<QStringView> parseForLinksToSkip(const Data::FString& _text)
{
    using ftype = core::data::format_type;
    std::vector<QStringView> result;
    for (auto [type, range, _] : _text.formatting().formats())
    {
        if (type == ftype::monospace || type == ftype::pre)
            result.push_back(Data::FStringView(_text, range.offset_, range.size_).string());
    }
    return result;
}

std::vector<QStringView> parseMarkdownForLinksToSkip(const QString& _text)
{
    constexpr auto sl = Data::singleBackTick();
    const auto ml = Data::tripleBackTick().toString();

    auto result = std::vector<QStringView>();
    const auto mcount = _text.count(ml);
    const auto scount = _text.count(sl);
    const QStringView textView = _text;
    if (mcount > 1)
    {
        qsizetype idxBeg = 0;
        while (idxBeg < textView.size())
        {
            const auto openingMD = idxBeg + textView.mid(idxBeg).indexOf(ml) + ml.size();
            if (openingMD >= textView.size())
                break;

            const auto closingMD = openingMD + textView.mid(openingMD).indexOf(ml);
            if (openingMD != -1 && closingMD != -1 && closingMD > openingMD)
            {
                result.push_back(textView.mid(openingMD, closingMD - openingMD));
                idxBeg += closingMD + ml.size() + 1;
            }
            else
            {
                idxBeg = openingMD + 1;
            }
        }
    }
    if (scount > 1)
    {
        const auto lines = _text.splitRef(QChar::LineFeed);
        for (const auto& l : lines)
        {
            auto first = -1, i = 0;
            const auto words = l.split(QChar::Space);
            for (const auto& w : words)
            {
                const auto startsWith = w.startsWith(sl);
                const auto endsWith = w.endsWith(sl);
                if (startsWith || endsWith)
                {
                    if (first == -1)
                    {
                        if (startsWith && endsWith && w.size() > 1)
                            result.push_back(w);
                        else
                            first = i;
                    }
                    else
                    {
                        for (auto j = first; j <= i; ++j)
                            result.push_back(words.at(j));

                        first = -1;
                    }
                }
                ++i;
            }
        }
    }

    return result;
}

ParseResult parseTextCommon(const Data::FString& _text, const bool _allowSnippet, const bool _forcePreview, const std::vector<QStringView>& _linksToSkip)
{
    ParseResult result;

    Utils::UrlParser parser;
    registerFixedUrls(parser);

    ChunkIterator it(_text, parser);
    while (it.hasNext())
    {
        auto chunk = it.current(_allowSnippet, _forcePreview);
        if (!_linksToSkip.empty() && chunk.Type_ != TextChunk::Type::Text && chunk.Type_ != TextChunk::Type::FormattedText && chunk.Type_ != TextChunk::Type::Pre)
        {
            for (const auto& m : _linksToSkip)
            {
                if (m.contains(chunk.getPlainText()))
                {
                    chunk.Type_ = chunk.formattedText_.hasFormatting() ? TextChunk::Type::FormattedText : TextChunk::Type::Text;
                    break;
                }
            }
        }

        if (chunk.Type_ == TextChunk::Type::GenericLink)
        {
            ++result.snippetsCount;
            if (result.snippetsCount == 1)
                result.snippetUrl_ = chunk.getFText();

            // append link if either there is more than one snippet, link is not last or message contains media
            it.next();
            if (it.hasNext() || result.snippetsCount > 1 || result.mediaCount > 0)
            {
                result.linkInsideText = true;
                result.chunks.emplace_back(std::move(chunk));
            }

            if (!it.hasNext() && result.snippetsCount == 1 && result.mediaCount == 0)
                result.hasTrailingLink = true;

            continue;
        }
        else
        {
            if (isMediaType(chunk.Type_))
                ++result.mediaCount;

            if (chunk.Type_ == TextChunk::Type::ImageLink && !_allowSnippet)
                chunk.Type_ = TextChunk::Type::GenericLink;

            result.chunks.emplace_back(std::move(chunk));
        }

        it.next();
    }

    result.mergeChunks();
    return result;
}

ParseResult parseText(const Data::FString& _text, const bool _allowSnippet, const bool _forcePreview)
{
    return parseTextCommon(_text, _allowSnippet, _forcePreview, parseForLinksToSkip(_text));
}


//! Add plaintext TextChunk if format is empty or add formatted TextChunk otherwise (to keep old backticks parsing)
void addDescriptionChunk(const Data::FString& _description, std::list<Ui::ComplexMessage::TextChunk>& _chunks)
{
    if (_description.hasFormatting())
    {
        auto descriptionMsg = parseText(_description, false, false);
        std::move(descriptionMsg.chunks.begin(), descriptionMsg.chunks.end(), std::back_inserter(_chunks));
    }
    else
    {
        _chunks.emplace_back(TextChunk::Type::Text, _description.string());
    }
}

ParseResult parseQuote(const Data::Quote& _quote, const Data::MentionMap& _mentions, const bool _allowSnippet, const bool _forcePreview)
{
    ParseResult result;
    const auto& text = _quote.text_;

    if (_quote.isSticker())
    {
        TextChunk chunk(TextChunk::Type::Sticker, text.string());
        chunk.Sticker_ = HistoryControl::StickerInfo::Make(_quote.setId_, _quote.stickerId_);
        result.chunks.emplace_back(std::move(chunk));
    }
    else if (!_quote.description_.isEmpty())
    {
        result = parseText(_quote.url_, _allowSnippet, _forcePreview);
        addDescriptionChunk(_quote.description_, result.chunks);
    }
    else if (!text.hasFormatting() && text.startsWith(ql1c('>')))
    {
        result.chunks.emplace_back(TextChunk::Type::Text, text.string());
    }
    else
    {
        result = parseText(text, _allowSnippet, _forcePreview);
    }

    result.sharedContact_ = _quote.sharedContact_;
    result.geo_ = _quote.geo_;
    result.poll_ = _quote.poll_;
    result.task_ = _quote.task_;
    result.sourceText_ = text.string();

    return result;
}

enum class QuoteType
{
    None,
    Quote,
    Forward
};

std::vector<GenericBlock*> createBlocks(const ParseResult& _parseRes, ComplexMessageItem* _parent, const int64_t _id, const int64_t _prev, int _quotesCount, QuoteType _quoteType = QuoteType::None)
{
    std::vector<GenericBlock*> blocks;
    blocks.reserve(_parseRes.chunks.size());

    const auto isQuote = _quoteType == QuoteType::Quote;
    const auto isForward = _quoteType == QuoteType::Forward;
    const auto bigEmojiAllowed = !isQuote && !isForward && Ui::get_gui_settings()->get_value<bool>(settings_allow_big_emoji, settings_allow_big_emoji_default());
    const auto emojiSizeType = bigEmojiAllowed ? TextRendering::EmojiSizeType::ALLOW_BIG : TextRendering::EmojiSizeType::REGULAR;

    if (_parseRes.poll_ && !isForward)
    {
        blocks.push_back(new PollBlock(_parent, *_parseRes.poll_, convertPollQuoteText(_parseRes.sourceText_)));
        return blocks;
    }

    if (_parseRes.task_)
    {
        blocks.push_back(new TaskBlock(_parent, *_parseRes.task_, convertPollQuoteText(_parseRes.sourceText_)));
        return blocks;
    }

    for (const auto& chunk : _parseRes.chunks)
    {
        switch (chunk.Type_)
        {
            case TextChunk::Type::Text:
            case TextChunk::Type::GenericLink:
                if (auto plainText = chunk.getPlainText(); !plainText.trimmed().isEmpty())
                    blocks.emplace_back(new TextBlock(_parent, plainText.toString(), emojiSizeType));
                break;

            case TextChunk::Type::FormattedText:
                if (auto text = chunk.getFView(); !text.isEmpty())
                    blocks.emplace_back(new TextBlock(_parent, text, emojiSizeType));
                break;
            case TextChunk::Type::Pre:
                if (auto text = chunk.getFView(); !text.isEmpty())
                    blocks.emplace_back(new CodeBlock(_parent, text));
                break;

            case TextChunk::Type::ImageLink:
                if (auto block = _parent->addSnippetBlock(chunk.getFView(), _parseRes.linkInsideText, estimatedTypeFromExtension(chunk.ImageType_), _quotesCount + blocks.size(), isQuote || isForward))
                    blocks.push_back(block);
                break;

            case TextChunk::Type::FileSharingImage:
            case TextChunk::Type::FileSharingImageSticker:
            case TextChunk::Type::FileSharingGif:
            case TextChunk::Type::FileSharingGifSticker:
            case TextChunk::Type::FileSharingVideo:
            case TextChunk::Type::FileSharingVideoSticker:
            case TextChunk::Type::FileSharingLottieSticker:
            case TextChunk::Type::FileSharingGeneral:
                blocks.push_back(chunk.FsInfo_ ? new FileSharingBlock(_parent, chunk.FsInfo_, getFileSharingContentType(chunk.Type_))
                    : new FileSharingBlock(_parent, chunk.getPlainText().toString(), getFileSharingContentType(chunk.Type_)));
                break;

            case TextChunk::Type::FileSharingPtt:
                blocks.push_back(chunk.FsInfo_ ? new PttBlock(_parent, chunk.FsInfo_, chunk.FsInfo_->duration() ? int32_t(*(chunk.FsInfo_->duration())) : chunk.DurationSec_, _id, _prev)
                    : new PttBlock(_parent, chunk.getPlainText().toString(), chunk.DurationSec_, _id, _prev));
                break;

            case TextChunk::Type::Sticker:
                blocks.push_back(new StickerBlock(_parent, chunk.Sticker_));
                break;

            case TextChunk::Type::ProfileLink:
                blocks.push_back(new ProfileBlock(_parent, chunk.getPlainText().toString()));
                break;

            case TextChunk::Type::Junk:
                break;

            default:
                im_assert(!"unexpected chunk type");
                break;
        }
    }

    // create snippet
    if (_parseRes.snippetsCount == 1 && _parseRes.mediaCount == 0)
    {
        const auto estimatedType = _parseRes.geo_ ? SnippetBlock::EstimatedType::Geo : SnippetBlock::EstimatedType::Article;
        auto block = _parent->addSnippetBlock(_parseRes.snippetUrl_, _parseRes.linkInsideText, estimatedType, _quotesCount + blocks.size(), isQuote || isForward);
        if (block)
            blocks.push_back(block);
    }

    if (_parseRes.sharedContact_)
        blocks.push_back(new PhoneProfileBlock(_parent, _parseRes.sharedContact_->name_, _parseRes.sharedContact_->phone_, _parseRes.sharedContact_->sn_));

    if (_parseRes.poll_)
        blocks.push_back(new PollBlock(_parent, *_parseRes.poll_, convertPollQuoteText(_parseRes.sourceText_)));

    return blocks;
}

namespace ComplexMessageItemBuilder
{
    std::unique_ptr<ComplexMessageItem> makeComplexItem(QWidget* _parent, const Data::MessageBuddy& _msg, ForcePreview _forcePreview)
    {
        auto complexItem = std::make_unique<ComplexMessage::ComplexMessageItem>(_parent, _msg);

        const auto text = _msg.GetText();
        const auto& formattedText = _msg.getFormattedText();
        const auto& description = _msg.GetDescription();
        const auto isNotAuth = Logic::getRecentsModel()->isSuspicious(_msg.AimId_);
        const auto isOutgoing = _msg.IsOutgoing();
        const auto id = _msg.Id_;
        const auto prev = _msg.Prev_;
        const auto& mentions = _msg.Mentions_;
        const auto& quotes = _msg.Quotes_;
        const auto& filesharing = _msg.GetFileSharing();
        const auto& sticker = _msg.GetSticker();

        const auto mediaType = complexItem->getMediaType();
        const bool allowSnippet = config::get().is_on(config::features::snippet_in_chat)
            && (isOutgoing || !isNotAuth || mediaType == Ui::MediaType::mediaTypePhoto);

        const auto quotesCount = quotes.size();
        std::vector<QuoteBlock*> quoteBlocks;
        quoteBlocks.reserve(quotesCount);

        int i = 0;
        for (auto quote: quotes)
        {
            quote.isFirstQuote_ = (i == 0);
            quote.isLastQuote_ = (i == quotesCount - 1);
            quote.mentions_ = mentions;
            quote.isSelectable_ = (quotesCount == 1);

            const auto& parsedQuote = parseQuote(quote, mentions, allowSnippet, _forcePreview == ForcePreview::Yes);
            const auto parsedBlocks = createBlocks(parsedQuote, complexItem.get(), id, prev, 0, quote.isForward_ ? QuoteType::Forward : QuoteType::Quote);

            auto quoteBlock = new QuoteBlock(complexItem.get(), quote);

            for (auto& block : parsedBlocks)
                quoteBlock->addBlock(block);

            quoteBlocks.push_back(quoteBlock);
            ++i;
        }

        std::vector<GenericBlock*> messageBlocks;
        bool hasTrailingLink = false;
        bool hasLinkInText = false;

        if (sticker)
        {
            TextChunk chunk(TextChunk::Type::Sticker, text, QString(), -1);
            chunk.Sticker_ = sticker;

            ParseResult res;
            res.chunks.push_back(std::move(chunk));
            messageBlocks = createBlocks(res, complexItem.get(), id, prev, quoteBlocks.size());
        }
        else if (_msg.sharedContact_)
        {
            messageBlocks.push_back(new PhoneProfileBlock(complexItem.get(), _msg.sharedContact_->name_, _msg.sharedContact_->phone_, _msg.sharedContact_->sn_));
        }
        else if (_msg.geo_)
        {
            messageBlocks.push_back(new SnippetBlock(complexItem.get(), text, false, SnippetBlock::EstimatedType::Geo));
        }
        else if (_msg.task_)
        {
            messageBlocks.push_back(new TaskBlock(complexItem.get(), *_msg.task_));
        }
        else if (filesharing && !filesharing->HasUri())
        {
            auto type = TextChunk::Type::FileSharingGeneral;
            if (filesharing->getContentType().is_undefined() && !filesharing->GetLocalPath().isEmpty())
            {
                constexpr auto gifExt = u"gif";
                const auto ext = QFileInfo(filesharing->GetLocalPath()).suffix();
                if (ext.compare(gifExt, Qt::CaseInsensitive) == 0)
                    type = TextChunk::Type::FileSharingGif;
                else if (Utils::is_image_extension(ext))
                    type = TextChunk::Type::FileSharingImage;
                else if (Utils::is_video_extension(ext))
                    type = TextChunk::Type::FileSharingVideo;
            }
            else
            {
                switch (filesharing->getContentType().type_)
                {
                case core::file_sharing_base_content_type::image:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingImageSticker : TextChunk::Type::FileSharingImage;
                    break;
                case core::file_sharing_base_content_type::gif:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingGifSticker : TextChunk::Type::FileSharingGif;
                    break;
                case core::file_sharing_base_content_type::video:
                    type = filesharing->getContentType().is_sticker() ? TextChunk::Type::FileSharingVideoSticker : TextChunk::Type::FileSharingVideo;
                    break;
                case core::file_sharing_base_content_type::lottie:
                    type = TextChunk::Type::FileSharingLottieSticker;
                    break;
                case core::file_sharing_base_content_type::ptt:
                    type = TextChunk::Type::FileSharingPtt;
                    break;
                default:
                    break;
                }
            }

            TextChunk chunk(type, text);
            chunk.FsInfo_ = filesharing;

            ParseResult res;
            res.chunks.emplace_back(std::move(chunk));

            if (!description.isEmpty())
                addDescriptionChunk(description, res.chunks);

            messageBlocks = createBlocks(res, complexItem.get(), id, prev, quoteBlocks.size());
        }
        else
        {
            const Data::FString textToParse = description.isEmpty() ? formattedText : _msg.GetUrl();
            auto parsedMsg = parseText(textToParse, allowSnippet, _forcePreview == ForcePreview::Yes);
            if (!description.isEmpty())
                addDescriptionChunk(description, parsedMsg.chunks);
            messageBlocks = createBlocks(parsedMsg, complexItem.get(), id, prev, quoteBlocks.size());
            hasTrailingLink = parsedMsg.hasTrailingLink;
            hasLinkInText = parsedMsg.linkInsideText;
        }

        if (_msg.poll_)
            messageBlocks.push_back(new PollBlock(complexItem.get(), *_msg.poll_));

        if (!messageBlocks.empty())
        {
            for (auto q : quoteBlocks)
                q->setReplyBlock(messageBlocks.front());
        }

        const bool appendId = GetAppConfig().IsShowMsgIdsEnabled();
        const auto blockCount = quoteBlocks.size() + messageBlocks.size() + (appendId ? 1 : 0);

        IItemBlocksVec items;
        items.reserve(blockCount);

        if (appendId)
            items.insert(items.begin(), new DebugTextBlock(complexItem.get(), id, DebugTextBlock::Subtype::MessageId));

        items.insert(items.end(), quoteBlocks.begin(), quoteBlocks.end());
        items.insert(items.end(), messageBlocks.begin(), messageBlocks.end());

        if (!quoteBlocks.empty())
        {
            Data::FString sourceText;
            for (auto item : items)
                sourceText += item->getSourceText();

            complexItem->setSourceText(std::move(sourceText));
        }

        int index = 0;
        for (auto val : quoteBlocks)
            val->setMessagesCountAndIndex(blockCount, index++);

        complexItem->setItems(std::move(items));
        complexItem->setHasTrailingLink(hasTrailingLink);
        complexItem->setHasLinkInText(hasLinkInText);
        complexItem->setButtons(_msg.buttons_);
        complexItem->setHideEdit(_msg.hideEdit());
        complexItem->setUrlAndDescription(_msg.GetUrl(), description);

        if (GetAppConfig().WatchGuiMemoryEnabled())
            MessageItemMemMonitor::instance().watchComplexMsgItem(complexItem.get());

        complexItem->setIsUnsupported(_msg.isUnsupported());

        return complexItem;
    }

}

UI_COMPLEX_MESSAGE_NS_END
