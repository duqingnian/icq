#include "stdafx.h"

#include "SmartReplyItem.h"

#include "controls/TextUnit.h"

#include "cache/stickers/stickers.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"
#include "utils/PainterPath.h"
#include "utils/InterConnector.h"
#include "core_dispatcher.h"
#include "fonts.h"
#include "app_config.h"
#include "main_window/smiles_menu/SmilesMenu.h"

#include "controls/LottiePlayer.h"

#include "../common.shared/smartreply/smartreply_types.h"

namespace
{
    int textItemHorMargin(int _emojiCount)
    {
        switch (_emojiCount)
        {
        case 1:
            return Utils::scale_value(4);
        case 2:
            return Utils::scale_value(6);
        case 3:
            return Utils::scale_value(8);
        default:
            break;
        }

        return Utils::scale_value(12);
    }

    int textItemHeight() { return Utils::scale_value(44); }
    int textItemRadius() { return Utils::scale_value(17); }
    int textItemBubbleHorMargin() { return Utils::scale_value(2); }

    const QFont& getTextItemFont()
    {
        static const auto font = Fonts::appFontScaled(16);
        return font;
    }

    int stickerBoxSize() { return Utils::scale_value(62); }
    int stickerSize() { return Utils::scale_value(54); }

    QSize gifLabelSize() { return Utils::scale_value(QSize(18, 12)); }

    constexpr std::chrono::milliseconds getLongtapTimeout() noexcept { return std::chrono::milliseconds(500); }

    void paintDebugInfo(QPainter& _p, const QRect& _itemRect, const Data::SmartreplySuggest& _suggest)
    {
        if (Q_UNLIKELY(Ui::GetAppConfig().IsShowMsgIdsEnabled()))
        {
            _p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY));
            _p.setFont(Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold));

            const auto x = _itemRect.width();
            const auto y = _itemRect.height();
            auto stickerData = std::get_if<Utils::FileSharingId>(&_suggest.getData());
            const QString text = (_suggest.isStickerType() && stickerData ? (stickerData->fileId_ % (stickerData->sourceId_ ? *stickerData->sourceId_ : QString())) : QString()) % QChar::LineFeed % QString::number(_suggest.getMsgId());
            Utils::drawText(_p, QPointF(x, y), Qt::AlignRight | Qt::AlignBottom, text);
        }
    }
}

namespace Ui
{
    SmartReplyItem::SmartReplyItem(QWidget* _parent, const Data::SmartreplySuggest& _suggest)
        : ClickableWidget(_parent)
        , suggest_(_suggest)
    {
        connect(this, &ClickableWidget::clicked, this, &SmartReplyItem::onClicked);
    }

    void SmartReplyItem::setFlat(const bool _flat)
    {
        if (isFlat_ != _flat)
        {
            isFlat_ = _flat;
            update();
        }
    }

    void SmartReplyItem::setUnderlayVisible(const bool _visible)
    {
        if (needUnderlay_ != _visible)
        {
            needUnderlay_ = _visible;
            update();
        }
    }

    int64_t SmartReplyItem::getMsgId() const noexcept
    {
        return suggest_.getMsgId();
    }

    void SmartReplyItem::onClicked() const
    {
        Q_EMIT selected(suggest_, QPrivateSignal());
    }

    SmartReplySticker::SmartReplySticker(QWidget* _parent, const Data::SmartreplySuggest& _suggest)
        : SmartReplyItem(_parent, _suggest)
    {
        im_assert(std::get_if<Utils::FileSharingId>(&_suggest.getData()));

        setFixedSize(stickerBoxSize(), stickerBoxSize());

        connect(this, &ClickableWidget::hoverChanged, this, qOverload<>(&SmartReplySticker::update));
        connect(this, &ClickableWidget::pressChanged, this, qOverload<>(&SmartReplySticker::update));

        connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, [this](qint32, const Utils::FileSharingId& _stickerId)
        {
            if (_stickerId == getId())
            {
                const auto sticker = Stickers::getSticker(getId());
                if (!sticker || sticker->isFailed())
                    Q_EMIT deleteMe();
                else
                    update();
            }
        });

        const auto sticker = Stickers::getSticker(getId());
        if (sticker && sticker->isLottie())
            connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, &SmartReplySticker::onStickerLoaded);

        prepareImage();

        longtapTimer_.setSingleShot(true);
        longtapTimer_.setInterval(getLongtapTimeout());
        connect(&longtapTimer_, &QTimer::timeout, this, [this]() { Q_EMIT showPreview(suggest_.getData()); });
    }

    void SmartReplySticker::onVisibilityChanged(bool _visible)
    {
        if (lottie_)
            lottie_->onVisibilityChanged(_visible);
    }

    void SmartReplySticker::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        auto drawDebug = qScopeGuard([&p, this](){ paintDebugInfo(p, rect(), getSuggest()); });

        if (const auto bgColor = getUnderlayColor(); bgColor.isValid())
        {
            p.setPen(Qt::NoPen);
            p.setBrush(bgColor);

            const qreal radius = qreal(Utils::scale_value(8));
            p.drawRoundedRect(rect(), radius, radius);
        }

        if (lottie_)
            return;

        if (prepareImage())
        {
            const auto x = (width() - image_.width() / Utils::scale_bitmap_ratio()) / 2;
            const auto y = (height() - image_.height() / Utils::scale_bitmap_ratio()) / 2;
            p.drawPixmap(x, y, image_);
        }

        const auto sticker = Stickers::getSticker(getId());
        if (sticker && sticker->isGif())
        {
            const auto size = gifLabelSize();

            const auto x = width() - (width() - stickerSize()) / 2 - size.width() - Utils::scale_value(1);
            const auto y = height() - (height() - stickerSize()) / 2 - size.height() - Utils::scale_value(1);

            static auto icon = Utils::StyledPixmap(qsl(":/smiles_menu/gif_label_small"), gifLabelSize());
            p.drawPixmap(x, y, icon.actualPixmap());
        }
    }

    QColor SmartReplySticker::getUnderlayColor()
    {
        const bool active = isEnabled() && (isHovered() || isPressed());
        if (isFlat())
        {
            if (active)
                return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);
        }
        else if (isUnderlayVisible())
        {
            if (active)
                return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);

            return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY, 0.8);
        }
        else if (active)
        {
            return Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ULTRALIGHT_INVERSE);
        }

        return QColor();
    }

    bool SmartReplySticker::prepareImage()
    {
        const auto& data = Stickers::getStickerData(getId(), core::sticker_size::small);
        if (data.isPixmap())
        {
            if (!image_.isNull())
                return true;

            const auto& pm = data.getPixmap();
            if (!pm.isNull())
            {
                const QSize imageSize(Utils::scale_bitmap(QSize(stickerSize(), stickerSize())));
                image_ = data.getPixmap().scaled(imageSize.width(), imageSize.height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                Utils::check_pixel_ratio(image_);

                return true;
            }
        }
        else if (data.isLottie())
        {
            if (lottie_)
                return true;

            if (const auto& path = data.getLottiePath(); !path.isEmpty())
            {
                QRect playerRect(0, 0, stickerSize(), stickerSize());
                playerRect.moveCenter(rect().center());
                lottie_ = new LottiePlayer(this, path, playerRect);
                return true;
            }
        }
        return false;
    }

    const Utils::FileSharingId& SmartReplySticker::getId() const
    {
        static const Utils::FileSharingId empty;
        auto fileSharingId = std::get_if<Utils::FileSharingId>(&suggest_.getData());
        return fileSharingId ? *fileSharingId : empty;
    }

    void SmartReplySticker::onStickerLoaded(int _error, const Utils::FileSharingId& _id)
    {
        if (_error == 0 || _id == getId())
            prepareImage();
    }

    void SmartReplySticker::mousePressEvent(QMouseEvent* _e)
    {
        if (_e->button() == Qt::LeftButton)
            longtapTimer_.start();

        SmartReplyItem::mousePressEvent(_e);
    }

    void SmartReplySticker::mouseReleaseEvent(QMouseEvent* _e)
    {
        longtapTimer_.stop();

        if (isPreviewVisible())
        {
            Q_EMIT hidePreview();
            setHovered(false);
        }
        update();

        SmartReplyItem::mouseReleaseEvent(_e);
    }

    void SmartReplySticker::mouseMoveEvent(QMouseEvent* _e)
    {
        if (isPressed() && isPreviewVisible())
            Q_EMIT mouseMoved(_e->globalPos());

        SmartReplyItem::mouseMoveEvent(_e);
    }

    SmartReplyText::SmartReplyText(QWidget* _parent, const Data::SmartreplySuggest& _suggest)
        : SmartReplyItem(_parent, _suggest)
        , hasMarginLeft_(true)
        , hasMarginRight_(true)
    {
        auto text = std::get_if<QString>(&suggest_.getData());
        im_assert(text && !text->isEmpty());

        textUnit_ = TextRendering::MakeTextUnit(text ? *text : QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS, TextRendering::EmojiSizeType::SMARTREPLY);
        TextRendering::TextUnit::InitializeParameters params{ getTextItemFont(), getTextColor() };
        params.maxLinesCount_ = 1;
        params.lineBreak_ = TextRendering::LineBreakType::PREFER_SPACES;
        params.emojiSizeType_ = TextRendering::EmojiSizeType::SMARTREPLY;
        textUnit_->init(params);
        textUnit_->evaluateDesiredSize();

        recalcSize();

        connect(this, &ClickableWidget::hoverChanged, this, &SmartReplyText::updateColors);
        connect(this, &ClickableWidget::pressChanged, this, &SmartReplyText::updateColors);
    }

    void SmartReplyText::setHasMargins(const bool _hasLeft, const bool _hasRight)
    {
        if (hasMarginLeft_ != _hasLeft || hasMarginRight_ != _hasRight)
        {
            hasMarginLeft_ = _hasLeft;
            hasMarginRight_ = _hasRight;
            recalcSize();
            update();
        }
    }

    void SmartReplyText::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(Qt::NoPen);

        if (!isFlat())
            Utils::drawBubbleShadow(p, bubble_);

        p.fillPath(bubble_, getBubbleColor());
        textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);

        paintDebugInfo(p, rect(), getSuggest());
    }

    Styling::ThemeColorKey SmartReplyText::getTextColor() const
    {
        if (isEnabled())
        {
            if (isPressed())
                return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_ACTIVE };
            else if (isHovered())
                return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY_HOVER };
        }

        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    QColor SmartReplyText::getBubbleColor() const
    {
        if (isFlat())
        {
            if (isEnabled())
            {
                if (isPressed())
                    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_ACTIVE);
                else if (isHovered())
                    return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_HOVER);
            }

            return Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        }

        if (isEnabled())
        {
            if (isPressed())
                return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);
            else if (isHovered())
                return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
        }

        return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
    }

    void SmartReplyText::updateColors()
    {
        textUnit_->setColor(getTextColor());
        update();
    }

    void SmartReplyText::recalcSize()
    {
        const auto bubbleSize = getBubbleSize();
        const auto margins = (hasMarginLeft_ ? textItemBubbleHorMargin() : 0) + (hasMarginRight_ ? textItemBubbleHorMargin() : 0);
        setFixedSize(bubbleSize.width() + margins, bubbleSize.height() + Utils::getShadowMargin() * 2);

        const auto bubbleX = hasMarginLeft_ ? textItemBubbleHorMargin() : 0;
        const auto bubbleY = (rect().height() - bubbleSize.height()) / 2;
        const auto bubbleRect = QRect(QPoint(bubbleX, bubbleY), bubbleSize);
        bubble_ = Utils::renderMessageBubble(bubbleRect, textItemRadius());

        const auto emojiCount = textUnit_->getEmojiCount();
        const auto shift = emojiCount > 0 ? (emojiCount == 3 ? 2 : 1) : 0;
        const auto textX = bubbleRect.left() + textItemHorMargin(emojiCount);
        const auto textY = bubbleRect.center().y() - Utils::scale_value(shift);
        textUnit_->setOffsets(textX, textY);
    }

    QSize SmartReplyText::getBubbleSize() const
    {
        const auto textWidth = textUnit_->desiredWidth();
        const auto margins = 2 * textItemHorMargin(textUnit_->getEmojiCount());
        return QSize(textWidth + margins, textItemHeight());
    }

    std::unique_ptr<SmartReplyItem> makeSmartReplyItem(const Data::SmartreplySuggest& _suggest)
    {
        switch (_suggest.getType())
        {
        case core::smartreply::type::sticker:
        case core::smartreply::type::sticker_by_text:
            if (auto fileSharingId = std::get_if<Utils::FileSharingId>(&_suggest.getData()))
            {
                if (const auto sticker = Stickers::getSticker(*fileSharingId); sticker && sticker->isFailed())
                    return nullptr;
            }
            else
            {
                return nullptr;
            }

            return std::make_unique<SmartReplySticker>(nullptr, _suggest);

        case core::smartreply::type::text:
            return std::make_unique<SmartReplyText>(nullptr, _suggest);

        default:
            break;
        }

        return nullptr;
    }
}