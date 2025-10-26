#include "stdafx.h"

#include "../../../utils/log/log.h"
#include "../../../utils/InterConnector.h"
#include "../../../utils/utils.h"

#include "../ActionButtonWidget.h"
#include "../MessageStatusWidget.h"
#include "../MessageStyle.h"

#include "ComplexMessageItem.h"
#include "IItemBlock.h"
#include "IItemBlockLayout.h"


#include "ComplexMessageItemLayout.h"
#include "../../../controls/TextUnit.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

ComplexMessageItemLayout::ComplexMessageItemLayout(ComplexMessageItem *parent)
    : QLayout(parent)
    , Item_(parent)
    , WidgetHeight_(-1)
    , bubbleBlockHorLeftPadding_(MessageStyle::getBubbleHorPadding())
    , bubbleBlockHorRightPadding_(MessageStyle::getBubbleHorPadding())
{
    im_assert(Item_);
}

ComplexMessageItemLayout::~ComplexMessageItemLayout() = default;

QRect ComplexMessageItemLayout::evaluateAvatarRect(const QRect& _widgetContentLtr) const
{
    im_assert(_widgetContentLtr.width() > 0 && _widgetContentLtr.height() >= 0);

    const auto avatarSize = MessageStyle::getAvatarSize();
    return QRect(_widgetContentLtr.topLeft(), QSize(avatarSize, avatarSize + Utils::scale_value(4)));
}

QRect ComplexMessageItemLayout::evaluateBlocksBubbleGeometry(const bool isMarginRequired, const QRect& _blocksContentLtr) const
{
    QMargins margins;
    if (isMarginRequired)
    {
        margins.setLeft(getBubbleLeftPadding());
        margins.setRight(getBubbleRightPadding());
        margins.setTop(MessageStyle::getBubbleVerPadding());
        margins.setBottom(MessageStyle::getBubbleVerPadding());
    }
    else if (isHeaderOrSticker())
    {
        margins.setBottom(MessageStyle::getBubbleVerPadding());
    }

    auto bubbleGeometry = _blocksContentLtr + margins;
    const auto validBubbleHeight = std::max(bubbleGeometry.height(), MessageStyle::getMinBubbleHeight());

    bubbleGeometry.setHeight(validBubbleHeight);

    return bubbleGeometry;
}

QRect ComplexMessageItemLayout::evaluateBlocksContainerLtr(const QRect& _avatarRect, const QRect& _widgetContentLtr) const
{
    auto left = (isOutgoingPosition() || !_avatarRect.isValid())
        ? _widgetContentLtr.left()
        : _avatarRect.right() + 1 + MessageStyle::getAvatarRightMargin();
    auto right = _widgetContentLtr.right();

    if (const auto desiredWidth = Item_->desiredWidth(); desiredWidth != 0)
    {
        const auto currentWidth = right - left;
        if (const auto diff = currentWidth - desiredWidth; diff > 0)
        {
            if (isOutgoingPosition())
                left += diff;
            else
                right -= diff;
        }
    }

    QRect blocksContainerLtr = _widgetContentLtr;

    blocksContainerLtr.setLeft(left);
    blocksContainerLtr.setRight(right);

    im_assert(blocksContainerLtr.width() > 0);

    return blocksContainerLtr;
}

QRect ComplexMessageItemLayout::evaluateBlockLtr(
    const QRect &blocksContentLtr,
    IItemBlock *block,
    const int32_t blockY,
    const bool isBubbleRequired)
{
    im_assert(block);
    im_assert(blockY >= 0);

    auto blocksContentWidth = blocksContentLtr.width();

    auto blockLeft = blocksContentLtr.left();

    QRect blockLtr(
        blockLeft,
        blockY,
        blocksContentWidth,
        0);

    if (isBubbleRequired && !block->isSizeLimited())
        return blockLtr;

    const auto blockSize = block->blockSizeForMaxWidth(blocksContentWidth);

    if (blockSize.isEmpty())
        return blockLtr;

    if (isOutgoingPosition() && !isBubbleRequired)
    {
        const auto outgoingBlockLeft = blockLtr.right() + 1 - blockSize.width();
        blockLeft = std::max(outgoingBlockLeft, blockLeft);
    }

    im_assert(blockLeft > 0);
    blockLtr.setLeft(blockLeft);

    if (block->isSizeLimited() && blockSize.width() < blockLtr.width())
        blockLtr.setWidth(blockSize.width());

    im_assert(blockLtr.width() > 0);
    return blockLtr;
}

QRect ComplexMessageItemLayout::evaluateSenderContentLtr(const QRect& _widgetContentLtr, const QRect& _avatarRect) const
{
    im_assert(_widgetContentLtr.width() > 0 && _widgetContentLtr.height() >= 0);

    if (Item_->isSenderVisible())
    {
        auto blockOffsetHor = _widgetContentLtr.left() + getBubbleLeftPadding();
        if (_avatarRect.isValid())
            blockOffsetHor += _avatarRect.width() + MessageStyle::getAvatarRightMargin();

        const auto blockOffsetVer = _widgetContentLtr.top() + MessageStyle::getSenderTopPadding();
        Item_->Sender_->setOffsets(blockOffsetHor, blockOffsetVer + MessageStyle::getSenderBaseline());

        const auto botMargin = isHeaderOrSticker()
            ? MessageStyle::getSenderBottomMarginLarge()
            : MessageStyle::getSenderBottomMargin();

        return QRect(
            blockOffsetHor,
            blockOffsetVer,
            Item_->getSenderDesiredWidth(),
            MessageStyle::getSenderHeight() + botMargin
        );
    }

    return QRect();
}

QRect ComplexMessageItemLayout::evaluateWidgetContentLtr(const int32_t _widgetWidth) const
{
    const auto isOutgoing = isOutgoingPosition();
    const auto isChat = Item_->isChat();
    const auto avatarWidth = MessageStyle::getAvatarSize() + MessageStyle::getAvatarRightMargin();

    auto widgetContentLeftMargin = MessageStyle::getLeftMargin(isOutgoing, _widgetWidth);
    if (!isOutgoing)
        widgetContentLeftMargin -= avatarWidth;

    const auto widgetContentRightMargin = MessageStyle::getRightMargin(isOutgoing, _widgetWidth, Item_->isMultiselected());

    im_assert(widgetContentLeftMargin > 0);
    im_assert(widgetContentRightMargin > 0);

    auto widgetContentWidth = _widgetWidth - (widgetContentLeftMargin + widgetContentRightMargin);

    auto itemMaxWidth = Item_->getMaxWidth();
    if (itemMaxWidth > 0 && Item_->isBubbleRequired())
        itemMaxWidth += getBubbleLeftPadding() + getBubbleRightPadding();

    const bool onlyTextAndCodeBlocks = Item_->hasOnlyTextAndExpandableBlocks();

    auto messageMaxWidth = onlyTextAndCodeBlocks ? MessageStyle::getTextCodeMessageMaxWidth() : MessageStyle::getMessageMaxWidth();
    if (itemMaxWidth > 0)
        messageMaxWidth = std::min(itemMaxWidth, messageMaxWidth);
    if (isChat && !isOutgoing)
        messageMaxWidth += avatarWidth;

    const auto maxWidth = std::min(widgetContentWidth, messageMaxWidth);

    if (widgetContentWidth > maxWidth)
    {
        if (isOutgoing)
            widgetContentLeftMargin += (widgetContentWidth - maxWidth);
        widgetContentWidth = maxWidth;
    }

    return QRect(
        widgetContentLeftMargin,
        MessageStyle::getTopMargin(Item_->hasTopMargin()),
        widgetContentWidth,
        0);
}

void ComplexMessageItemLayout::setGeometry(const QRect &r)
{
    QLayout::setGeometry(r);

    const auto sizeBefore = sizeHint();

    setGeometryInternal(r.width());

    LastGeometry_ = r;

    if (sizeBefore != sizeHint())
        update();
}

void ComplexMessageItemLayout::addItem(QLayoutItem* /*item*/)
{
}

QLayoutItem* ComplexMessageItemLayout::itemAt(int /*index*/) const
{
    return nullptr;
}

QLayoutItem* ComplexMessageItemLayout::takeAt(int /*index*/)
{
    return nullptr;
}

int ComplexMessageItemLayout::count() const
{
    return 0;
}

QSize ComplexMessageItemLayout::sizeHint() const
{
    const auto height = std::max(
        WidgetHeight_,
        MessageStyle::getMinBubbleHeight());

    return QSize(-1, height);
}

void ComplexMessageItemLayout::invalidate()
{
    QLayoutItem::invalidate();
}

bool ComplexMessageItemLayout::isOverAvatar(const QPoint &pos) const
{
    return getAvatarRect().contains(pos);
}

QRect ComplexMessageItemLayout::getAvatarRect() const
{
    return AvatarRect_;
}

const QRect& ComplexMessageItemLayout::getBlocksContentRect() const
{
    return BlocksContentRect_;
}

const QRect& ComplexMessageItemLayout::getBubbleRect() const
{
    return BubbleRect_;
}

QPoint ComplexMessageItemLayout::getShareButtonPos(const IItemBlock &block, const bool isBubbleRequired) const
{
    return block.getShareButtonPos(isBubbleRequired, BubbleRect_);
}

void ComplexMessageItemLayout::onBlockSizeChanged()
{
    if (LastGeometry_.width() <= 0)
        return;

    const auto sizeBefore = sizeHint();

    setGeometryInternal(LastGeometry_.width());

    const auto sizeAfter = sizeHint();

    if (sizeBefore != sizeAfter)
        update();
}

QPoint ComplexMessageItemLayout::getInitialTimePosition() const
{
    return initialTimePosition_;
}

int32_t ComplexMessageItemLayout::getBubbleLeftPadding() const
{
    return bubbleBlockHorLeftPadding_ + Item_->getBubbleHorMarginAdjust().left_;
}

int32_t ComplexMessageItemLayout::getBubbleRightPadding() const
{
    return bubbleBlockHorRightPadding_ + Item_->getBubbleHorMarginAdjust().right_;
}

void ComplexMessageItemLayout::updateSize(int _width)
{
    setGeometryInternal(_width);
    update();
}

bool ComplexMessageItemLayout::hasSeparator(const IItemBlock *block) const
{
    const auto &blocks = Item_->Blocks_;

    auto blockIter = std::find(blocks.cbegin(), blocks.cend(), block);

    const auto isFirstBlock = (blockIter == blocks.begin());
    if (isFirstBlock)
        return false;

    auto prevBlockIter = blockIter;
    --prevBlockIter;

    const auto afterLastQuote =
        (*prevBlockIter)->getContentType() == IItemBlock::ContentType::Quote &&
        (*blockIter)->getContentType() != IItemBlock::ContentType::Quote;
    if (afterLastQuote)
        return false;

    const auto hasSeparator = ((*blockIter)->hasLeadLines() || (*prevBlockIter)->hasLeadLines());
    return hasSeparator;
}

bool ComplexMessageItemLayout::isOutgoingPosition() const
{
    im_assert(Item_);
    return Item_->isOutgoingPosition();
}

bool ComplexMessageItemLayout::isHeaderOrSticker() const
{
    return Item_->isHeaderRequired() || Item_->isSingleSticker();
}

QRect ComplexMessageItemLayout::setBlocksGeometry(
    const bool _isBubbleRequired,
    const QRect& _messageRect,
    const QRect& _senderRect)
{
    im_assert(_messageRect.width() > 0 && _messageRect.height() > 0);

    const auto topY = _senderRect.isValid() ? _senderRect.bottom() + 1 : _messageRect.top();
    auto blocksHeight = 0;
    auto blocksWidth = Item_->calcButtonsWidth(_messageRect.width());
    auto blocksLeft = isOutgoingPosition() ? _messageRect.left() + _messageRect.width() : _messageRect.left();

    auto& blocks = Item_->Blocks_;
    for (auto block : blocks)
    {
        im_assert(block);

        const auto blockSeparatorHeight = hasSeparator(block) ? MessageStyle::getBlocksSeparatorVertMargins(): 0;
        const auto blockY = topY + blocksHeight + blockSeparatorHeight;

        auto blockLtr = evaluateBlockLtr(_messageRect, block, blockY, _isBubbleRequired);

        auto widthAdjust = 0;
        if (block->getQuote().isForward_)
        {
            blockLtr.adjust(-MessageStyle::Quote::getFwdLeftOverflow(), 0, MessageStyle::Quote::getFwdLeftOverflow(), 0);
            widthAdjust = 2 * MessageStyle::Quote::getFwdLeftOverflow();
        }

        const auto blockGeometry = block->setBlockGeometry(blockLtr);

        const auto blockHeight = blockGeometry.height();

        blocksHeight += blockHeight + blockSeparatorHeight;

        if (block->getContentType() == IItemBlock::ContentType::Quote && blocks.back() != block)
            blocksHeight += (block->getQuote().isLastQuote_ ? MessageStyle::Quote::getSpacingAfterLastQuote() : MessageStyle::Quote::getQuoteSpacing());

        blocksWidth = std::max(blocksWidth, blockGeometry.width() - widthAdjust);

        if (isOutgoingPosition())
            blocksLeft = std::min(blocksLeft, blockGeometry.left());
        else
            blocksLeft = std::max(blocksLeft, blockGeometry.left());
    }
    if (Item_->TimeWidget_)
        blocksWidth = std::max(blocksWidth, Item_->TimeWidget_->width() - MessageStyle::getTimeLeftSpacing() / 2);

    for (auto block : blocks)
    {
        if (block->needStretchToOthers())
            block->stretchToWidth(blocksWidth);
    }

    if (_isBubbleRequired && !blocks.empty())
    {
        if (isOutgoingPosition() && blocksWidth < _messageRect.width())
        {
            const auto shift = _messageRect.width() - blocksWidth;
            for (auto block : blocks)
                block->shiftHorizontally(shift);
            blocksLeft += shift;
        }

        const auto lastBlock = blocks.back();
        const auto lastBlockManagesTime = lastBlock->managesTime();
        if (!lastBlockManagesTime && Item_->TimeWidget_ && !Item_->TimeWidget_->isUnderlayVisible())
        {
            const auto tw = MessageStyle::getTimeLeftSpacing() + Item_->TimeWidget_->width();
            const auto th = MessageStyle::getShiftedTimeTopMargin() + Item_->TimeWidget_->height();

            if (lastBlock->isNeedCheckTimeShift())
            {
                blocksHeight += th;
            }
            else
            {
                const auto lastBlockGeometry = lastBlock->getBlockGeometry();
                if (blocks.size() > 1)
                {
                    const bool enoughWidth = blocksWidth - lastBlockGeometry.width() > tw;
                    const bool rightAligned = enoughWidth && (blocksLeft + blocksWidth) - (lastBlockGeometry.right() + 1) < tw;
                    if (!enoughWidth || rightAligned)
                        blocksHeight += th;
                }
            }
        }
    }

    im_assert(blocksWidth >= 0);
    im_assert(blocksHeight >= 0);
    return QRect(blocksLeft, topY, blocksWidth, blocksHeight);
}

void ComplexMessageItemLayout::setGeometryInternal(const int32_t _widgetWidth)
{
    const QRect widgetContentLtr = evaluateWidgetContentLtr(_widgetWidth);

    if (widgetContentLtr.width() <= MessageStyle::getBubbleHorPadding())
        return;

    AvatarRect_ = (Item_->isChat() && !isOutgoingPosition()) ? evaluateAvatarRect(widgetContentLtr) : QRect(); // vertical pos invalid at this point

    const auto senderRect = evaluateSenderContentLtr(widgetContentLtr, AvatarRect_); // including top and bottom margin
    auto messageContentRect = evaluateBlocksContainerLtr(AvatarRect_, widgetContentLtr);

    const auto isBubbleRequired = Item_->isBubbleRequired();
    const auto isMarginRequired = Item_->isMarginRequired();

    if (isMarginRequired)
        messageContentRect.adjust(getBubbleLeftPadding(), MessageStyle::getBubbleVerPadding(), -getBubbleRightPadding(), 0);
    else if (isBubbleRequired)
        messageContentRect.adjust(0, MessageStyle::getBubbleVerPadding(), 0, 0);

    messageContentRect.setWidth(qMax(1, messageContentRect.width()));
    messageContentRect.setHeight(qMax(1, messageContentRect.height()));
    const auto blocksGeometry = setBlocksGeometry(isBubbleRequired, messageContentRect, senderRect);

    BlocksContentRect_ = blocksGeometry;

    messageContentRect.setHeight(blocksGeometry.height() + senderRect.height());

    if (!Item_->isSingleSticker())
    {
        if (isOutgoingPosition())
        {
            messageContentRect.setLeft(blocksGeometry.left());
        }
        else
        {
            if (Item_->canStretchWithSender() && Item_->isSenderVisible())
            {
                if (senderRect.width() < blocksGeometry.width())
                    messageContentRect.setWidth(blocksGeometry.width());
                else if (senderRect.width() >= blocksGeometry.width() && senderRect.width() <= messageContentRect.width())
                    messageContentRect.setWidth(senderRect.width());
            }
            else
                messageContentRect.setWidth(blocksGeometry.width());
        }
    }

    // If message is empty then content size is empty and there are too much asserts in debug
    messageContentRect.setWidth(qMax(1, messageContentRect.width()));
    messageContentRect.setHeight(qMax(1, messageContentRect.height()));

    if (Item_->isSenderVisible())
    {
        const auto adjust = (isHeaderOrSticker() || isBubbleRequired && !isMarginRequired)
            ? getBubbleLeftPadding() + getBubbleRightPadding()
            : 0;
        const auto availableWidth = messageContentRect.width() - adjust;
        Item_->Sender_->getHeight(availableWidth);
    }

    auto bubbleRect = evaluateBlocksBubbleGeometry(isMarginRequired, messageContentRect);

    if (isBubbleRequired && !isMarginRequired && Item_->isSenderVisible())
        bubbleRect.adjust(0, -MessageStyle::getBubbleVerPadding(), 0, 0);

    im_assert(bubbleRect.height() >= MessageStyle::getMinBubbleHeight());

    setTimePosition(isBubbleRequired ? bubbleRect : blocksGeometry);

    BubbleRect_ = bubbleRect;

    if (AvatarRect_.isValid())  // set valid vertical pos
    {
        if (Item_->isSingleSticker())
            AvatarRect_.moveBottom(blocksGeometry.bottom());
        else
            AvatarRect_.moveBottom(BubbleRect_.bottom());
    }

    WidgetHeight_ = (bubbleRect.bottom() + 1) + Item_->bottomOffset() + Item_->buttonsHeight();
    if (Item_->isSingleSticker() && Item_->hasButtons())
        WidgetHeight_ += MessageStyle::getBorderRadius();

    im_assert(WidgetHeight_ >= 0);

    Item_->onSizeChanged();
}

void ComplexMessageItemLayout::setTimePosition(const QRect& _availableGeometry)
{
    auto timeWidget = Item_->TimeWidget_;

    if (!Item_->TimeWidget_)
        return;

    if (!_availableGeometry.isValid() || !timeWidget || !timeWidget->size().isValid())
        return;

    const auto x = _availableGeometry.right() + 1 - timeWidget->width() - timeWidget->getHorMargin();
    const auto y = _availableGeometry.bottom() + 1 - timeWidget->height() - timeWidget->getVerMargin();

    timeWidget->move(x, y);
    initialTimePosition_ = timeWidget->pos();
}

UI_COMPLEX_MESSAGE_NS_END
