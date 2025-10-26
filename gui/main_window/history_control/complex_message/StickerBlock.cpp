#include "stdafx.h"

#include "../MessageStyle.h"
#include "styles/ThemesContainer.h"
#include "../../../utils/utils.h"
#include "../../../utils/InterConnector.h"
#include "../../../my_info.h"
#include "../../../core_dispatcher.h"
#include "../../../cache/stickers/stickers.h"
#include "../../mediatype.h"

#include "StickerBlockLayout.h"

#include "StickerBlock.h"
#include "ComplexMessageItem.h"
#include "FileSharingBlock.h"

using namespace Ui::Stickers;

namespace
{
    QSize getGhostSize()
    {
        return Utils::scale_value(QSize(160, 160));
    }

    auto getGhost()
    {
        return Utils::StyledPixmap(qsl("://themes/stickers/ghost_sticker"), getGhostSize());
    }

    core::sticker_size getStickerSize()
    {
        return core::sticker_size::large;
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

StickerBlock::StickerBlock(ComplexMessageItem* _parent,  const HistoryControl::StickerInfoSptr& _info)
    : GenericBlock(_parent, QT_TRANSLATE_NOOP("contact_list", "Sticker"), MenuFlagNone, false)
    , Info_(_info)
    , Layout_(nullptr)
    , LastSize_(getGhostSize())
{
    im_assert(Info_);

    Testing::setAccessibleName(this, u"AS HistoryPage messageSticker " % QString::number(_parent->getId()));

    Layout_ = new StickerBlockLayout();
    setLayout(Layout_);

    QuoteAnimation_->setSemiTransparent();
    setMouseTracking(true);

    connections_.reserve(4);
}

StickerBlock::~StickerBlock() = default;

IItemBlockLayout* StickerBlock::getBlockLayout() const
{
    return Layout_;
}

Data::FString StickerBlock::getSelectedText(const bool, const TextDestination) const
{
    return isSelected() ? QT_TRANSLATE_NOOP("contact_list", "Sticker") : QString();
}

Data::FString StickerBlock::getSourceText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

QString StickerBlock::formatRecentsText() const
{
    return QT_TRANSLATE_NOOP("contact_list", "Sticker");
}

Ui::MediaType StickerBlock::getMediaType() const
{
    return Ui::MediaType::mediaTypeSticker;
}

int StickerBlock::desiredWidth(int) const
{
    return LastSize_.width();
}

QRect StickerBlock::setBlockGeometry(const QRect &ltr)
{
    const auto r = GenericBlock::setBlockGeometry(ltr);
    Geometry_ = QRect(r.topLeft(), LastSize_);
    setGeometry(Geometry_);
    return Geometry_;
}

void StickerBlock::requestPinPreview()
{
    auto conn = std::make_shared<QMetaObject::Connection>();
    *conn = connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this,
        [conn, this](qint32 _error, const Utils::FileSharingId&, qint32 _setId, qint32 _stickerId)
    {
        if (int(Info_->SetId_) == _setId && int(Info_->StickerId_) == _stickerId)
        {
            if (_error == 0 && getParentComplexMessage())
            {
                if (const auto sticker = getSticker(Info_->SetId_, Info_->StickerId_))
                    Q_EMIT getParentComplexMessage()->pinPreview(sticker->getData(getStickerSize()).getPixmap());
            }
            disconnect(*conn);
        }
    });

    requestSticker();
}

void StickerBlock::drawBlock(QPainter& _p, const QRect& _rect, const QColor& _quoteColor)
{
    Utils::PainterSaver ps(_p);

    if (Sticker_.isNull())
    {
        if (!Placeholder_.actualPixmap().isNull())
        {
            const auto offset = (isOutgoing() ? (width() - getGhostSize().width()) : 0);
            _p.drawPixmap(QRect(QPoint(offset, 0), LastSize_), Placeholder_.cachedPixmap());
        }
    }
    else
    {
        updateStickerSize();

        const auto offset = (isOutgoing() ? (width() - LastSize_.width()) : 0);
        const auto rect = QRect(QPoint(offset, 0), LastSize_);

        _p.drawImage(rect, Sticker_);

        if (_quoteColor.isValid())
        {
            _p.setBrush(QBrush(_quoteColor));
            _p.drawRoundedRect(rect, Utils::scale_value(8), Utils::scale_value(8), Qt::RelativeSize);
        }
    }
}

void StickerBlock::initialize()
{
    connectStickerSignal(true);

    requestSticker();
}

void StickerBlock::connectStickerSignal(const bool _isConnected)
{
    for (const auto& c : connections_)
        disconnect(c);

    connections_.clear();

    if (_isConnected)
    {
        connections_.assign({
            connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated,  this, &StickerBlock::onSticker),
            connect(GetDispatcher(), &core_dispatcher::onStickers, this, &StickerBlock::onStickers)
        });
    }
}

void StickerBlock::loadSticker()
{
    im_assert(Sticker_.isNull());

    const auto sticker = getSticker(Info_->SetId_, Info_->StickerId_);
    if (!sticker)
        return;

    Sticker_ = sticker->getData(getStickerSize()).getPixmap().toImage();

    if (sticker->isFailed())
        initPlaceholder();

    if (Sticker_.isNull())
        return;

    Sticker_ = FileSharingBlock::scaledSticker(Sticker_);

    connectStickerSignal(false);

    setCursor(Qt::PointingHandCursor);
    updateGeometry();
    update();
}

void StickerBlock::requestSticker()
{
    im_assert(Sticker_.isNull());

    Ui::GetDispatcher()->getSticker(Info_->SetId_, Info_->StickerId_, getStickerSize());
}

void StickerBlock::updateStickerSize()
{
    auto stickerSize = Sticker_.size();
    if (Utils::is_mac_retina())
        stickerSize = QSize(stickerSize.width() / 2, stickerSize.height() / 2);

    if (LastSize_ != stickerSize)
    {
        LastSize_ = stickerSize;
        notifyBlockContentsChanged();
    }
}

void StickerBlock::initPlaceholder()
{
    if (Placeholder_.cachedPixmap().isNull())
    {
        Placeholder_ = getGhost();
        LastSize_ = getGhostSize();
    }
}

void StickerBlock::onSticker(const qint32 _error, const Utils::FileSharingId&, const qint32 _setId, const qint32 _stickerId)
{
    if (_stickerId <= 0)
        return;

    im_assert(_setId > 0);

    const auto isMySticker = ((qint32(Info_->SetId_) == _setId) && (qint32(Info_->StickerId_) == _stickerId));
    if (!isMySticker)
        return;

    if (_error == 0)
    {
        if (!Sticker_.isNull())
            return;

        loadSticker();
        return;
    }

    initPlaceholder();

    update();
}

void StickerBlock::onStickers()
{
    if (Sticker_.isNull())
        requestSticker();
}

const QImage& StickerBlock::getStickerImage() const
{
    return Sticker_;
}

bool StickerBlock::clicked(const QPoint& _p)
{
    QPoint mappedPoint = mapFromParent(_p, Layout_->getBlockGeometry());
    if (!rect().contains(mappedPoint))
        return true;

    Q_EMIT Utils::InterConnector::instance().stopPttRecord();
    Stickers::showStickersPack(Info_->SetId_, Stickers::StatContext::Chat);

    if (auto pack = Stickers::getSet(Info_->SetId_); pack && !pack->isPurchased())
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_sticker_tap_in_chat);

    return true;
}

Data::StickerId StickerBlock::getStickerId() const
{
    if (!Info_)
        return {};

    return Data::StickerId(Info_->SetId_, Info_->StickerId_);
}

UI_COMPLEX_MESSAGE_NS_END
