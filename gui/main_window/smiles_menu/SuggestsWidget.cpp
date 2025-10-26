#include "stdafx.h"

#include "SuggestsWidget.h"
#include "../../utils/utils.h"
#include "../../core_dispatcher.h"
#include "../../cache/stickers/stickers.h"
#include "../../controls/TooltipWidget.h"
#include "../../utils/InterConnector.h"
#include "../../main_window/MainWindow.h"
#include "../../styles/ThemeParameters.h"
#include "StickerPreview.h"

namespace
{
    int getStickersSpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    int getSuggestHeight() noexcept
    {
        return Utils::scale_value(70);
    }
}

using namespace Ui;
using namespace Stickers;

int Stickers::getStickerWidth()
{
    return Utils::scale_value(60);
}

StickersWidget::StickersWidget(QWidget* _parent)
    : StickersTable(_parent, -1, Stickers::getStickerWidth(), Stickers::getStickerWidth() + getStickersSpacing(), false)
    , stickerPreview_(nullptr)
{
    connect(&Stickers::getCache(), &Stickers::Cache::stickerUpdated, this, [this](qint32 _error, const Utils::FileSharingId& _stickerId)
    {
        if (_error == 0)
            redrawSticker(-1, _stickerId);
    });

    setFixedHeight(getSuggestHeight());
    setMouseTracking(true);

    connect(this, &StickersWidget::stickerPreview, this, &StickersWidget::onStickerPreview);
}

void StickersWidget::closeStickerPreview()
{
    if (!stickerPreview_)
        return;

    stickerPreview_->hide();
    delete stickerPreview_;
    stickerPreview_ = nullptr;
}

void StickersWidget::onStickerPreview(const Utils::FileSharingId& _stickerId)
{
    if (!stickerPreview_)
    {
        stickerPreview_ = new Smiles::StickerPreview(Utils::InterConnector::instance().getMainWindow()->getWidget(), -1, _stickerId, Smiles::StickerPreview::Context::Popup);
        stickerPreview_->setGeometry(Utils::InterConnector::instance().getMainWindow()->getWidget()->rect());
        stickerPreview_->show();
        stickerPreview_->raise();

        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::stickers_longtap_preview);

        QObject::connect(stickerPreview_, &Smiles::StickerPreview::needClose, this, &StickersWidget::closeStickerPreview, Qt::QueuedConnection); // !!! must be Qt::QueuedConnection
        QObject::connect(this, &StickersTable::stickerHovered, this, &StickersWidget::onStickerHovered);
    }
}

void StickersWidget::onStickerHovered(const int32_t _setId, const Utils::FileSharingId& _stickerId)
{
    if (stickerPreview_)
        stickerPreview_->showSticker(_stickerId);
}

void StickersWidget::init(const QString& _text)
{
    Stickers::Suggest suggest;
    Stickers::getSuggestWithSettings(_text, suggest);

    stickersArray_.reserve(suggest.size());

    const auto oldSize = stickersArray_.size();
    for (const auto& _sticker : suggest)
    {
        getStickerData(_sticker.fsId_, core::sticker_size::small);
        if (std::find(stickersArray_.begin(), stickersArray_.end(), _sticker.fsId_) == stickersArray_.end())
            stickersArray_.push_back(_sticker.fsId_);
    }

    if (stickersArray_.size() != oldSize)
    {
        setFixedWidth(itemSize_ * suggest.size());
        populateStickerWidgets();
    }
}

bool StickersWidget::resize(const QSize& _size, bool _force)
{
    needHeight_ = Stickers::getStickerWidth();

    columnCount_ = stickersArray_.size();

    rowCount_ = 1;

    return false;
}

void StickersWidget::mouseReleaseEvent(QMouseEvent* _e)
{
    if (stickerPreview_)
        closeStickerPreview();
    else
        StickersTable::mouseReleaseEventInternal(_e->pos());

    QWidget::mouseReleaseEvent(_e);
}

void StickersWidget::mousePressEvent(QMouseEvent* _e)
{
    StickersTable::mousePressEventInternal(_e->pos());

    QWidget::mousePressEvent(_e);
}

void StickersWidget::mouseMoveEvent(QMouseEvent* _e)
{
    StickersTable::mouseMoveEventInternal(_e->pos());

    QWidget::mouseMoveEvent(_e);
}

void StickersWidget::leaveEvent(QEvent* _e)
{
    StickersTable::leaveEventInternal();

    QWidget::leaveEvent(_e);
}

int32_t StickersWidget::getStickerPosInSet(const Utils::FileSharingId& _stickerId) const
{
    const auto it = std::find_if(stickersArray_.begin(), stickersArray_.end(), [&_stickerId](const auto& _s) { return _s == _stickerId; });
    if (it != stickersArray_.end())
        return std::distance(stickersArray_.begin(), it);
    return -1;
}

const stickersArray& StickersWidget::getStickerIds() const
{
    return stickersArray_;
}

void StickersWidget::setSelected(const std::pair<int32_t, Utils::FileSharingId>& _sticker)
{
    const auto stickerPosInSet = getStickerPosInSet(_sticker.second);
    if (stickerPosInSet >= 0)
        Q_EMIT stickerHovered(getStickerRect(stickerPosInSet));

    StickersTable::setSelected(_sticker);
}

void StickersWidget::clearStickers()
{
    stickersArray_.clear();
}

StickersSuggest::StickersSuggest(QWidget* _parent)
    : Styling::StyledWidget(_parent)
    , stickers_{ new StickersWidget(_parent) }
    , tooltip_{ new TooltipWidget(_parent, stickers_, Utils::scale_value(44), QMargins(Utils::scale_value(8), Utils::scale_value(4), Utils::scale_value(8), Utils::scale_value(4)), TooltipCompopent::All) }
    , keyboardActive_(false)
    , needScrollToTop_(false)
{
    setDefaultBackground();
    tooltip_->setCursor(Qt::PointingHandCursor);

    connect(stickers_, &StickersWidget::stickerSelected, this, &StickersSuggest::stickerSelected);
    connect(stickers_, &StickersWidget::stickerHovered, this, &StickersSuggest::stickerHovered);
    connect(tooltip_, &TooltipWidget::scrolledToLastItem, this, &StickersSuggest::scrolledToLastItem);
}

void StickersSuggest::showAnimated(const QString& _text, const QPoint& _p, const QSize& _maxSize, const QRect& _rect)
{
    stickers_->clearStickers();
    stickers_->init(_text);
    stickers_->setSelected(std::make_pair(-1, Utils::FileSharingId()));
    if (needScrollToTop_)
        tooltip_->scrollToTop();
    tooltip_->raise();
    tooltip_->showAnimated(_p, _maxSize, _rect);

    keyboardActive_ = false;
}

void StickersSuggest::cancelAnimation()
{
    tooltip_->cancelAnimation();
}

void StickersSuggest::updateStickers(const QString& _text, const QPoint& _p, const QSize& _maxSize, const QRect& _rect)
{
    stickers_->init(_text);
    stickers_->clearSelection();
    tooltip_->updateTooltip(_p, _maxSize, _rect);
}

void StickersSuggest::hideAnimated()
{
    tooltip_->hideAnimated();
}

bool StickersSuggest::isTooltipVisible() const
{
    return tooltip_->isVisible();
}

void Ui::Stickers::StickersSuggest::setArrowVisible(const bool _visible)
{
    tooltip_->setArrowVisible(_visible);
}

void StickersSuggest::stickerHovered(const QRect& _stickerRect)
{
    tooltip_->ensureVisible(_stickerRect.left(), _stickerRect.top(), _stickerRect.width(), _stickerRect.height());
}

void StickersSuggest::keyPressEvent(QKeyEvent* _event)
{
    _event->ignore();

    switch (_event->key())
    {
        case Qt::Key_Up:
        {
            stickers_->selectFirst();

            keyboardActive_ = true;

            _event->accept();

            break;
        }
        case Qt::Key_Down:
        {
            stickers_->clearSelection();

            keyboardActive_ = false;

            _event->accept();

            break;
        }
        case Qt::Key_Left:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (!selectedSticker.second.fileId_.isEmpty())
            {
                stickers_->selectLeft();

                _event->accept();
            }

            keyboardActive_ = true;

            break;
        }
        case Qt::Key_Right:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (!selectedSticker.second.fileId_.isEmpty())
            {
                stickers_->selectRight();

                _event->accept();
            }

            keyboardActive_ = true;

            break;
        }
        case Qt::Key_Return:
        case Qt::Key_Enter:
        {
            const auto selectedSticker = stickers_->getSelected();
            if (keyboardActive_ && !selectedSticker.second.fileId_.isEmpty())
            {
                Q_EMIT stickerSelected(selectedSticker.second);

                _event->accept();
            }
        }
    }
}