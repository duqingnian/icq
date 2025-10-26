#include "DebugTextBlock.h"

UI_COMPLEX_MESSAGE_NS_BEGIN

DebugTextBlock::DebugTextBlock(ComplexMessageItem* _parent, qint64 _id, Subtype _subtype)
    : TextBlock(_parent, QString::number(_id)),
      subtype_(_subtype)
{
    setMessageId(_id);
    Testing::setAccessibleName(this, u"AS HistoryPage debugMessageText " % QString::number(_id));
}

DebugTextBlock::~DebugTextBlock() = default;

DebugTextBlock::Subtype DebugTextBlock::getSubtype() const
{
    return subtype_;
}

qint64 DebugTextBlock::getMessageId() const
{
    return id_;
}

void DebugTextBlock::setMessageId(qint64 _id)
{
    id_ = _id;
}

Data::FString DebugTextBlock::getSourceText() const
{
    return {};
}

void DebugTextBlock::drawBlock(QPainter &_p, const QRect& _rect, const QColor& _quoteColor)
{
    TextBlock::drawBlock(_p, _rect, _quoteColor);
}

IItemBlock::ContentType  DebugTextBlock::getContentType() const
{
    return IItemBlock::ContentType::DebugText;
}

Data::Quote DebugTextBlock::getQuote() const
{
    return Data::Quote();
}

Data::FString DebugTextBlock::getSelectedText(const bool _isFullSelect, const TextDestination) const
{
    return TextBlock::getSelectedText(_isFullSelect);
}

UI_COMPLEX_MESSAGE_NS_END