#include "stdafx.h"

#include "OverlayTopChatWidget.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace Ui
{
    OverlayTopChatWidget::OverlayTopChatWidget(QWidget* _parent)
        : QWidget(_parent)
        , textUnit_(TextRendering::MakeTextUnit(QString(), {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS))
    {
        setStyleSheet(qsl("background: transparent; border-style: none;"));
        setAttribute(Qt::WA_TransparentForMouseEvents);

        TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(11), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } };
        params.align_ = TextRendering::HorAligment::CENTER;
        textUnit_->init(params);
    }

    OverlayTopChatWidget::~OverlayTopChatWidget() = default;

    void OverlayTopChatWidget::setBadgeText(const QString& _text)
    {
        if (_text != text_)
        {
            text_ = _text;
            textUnit_->setText(text_);
            textUnit_->evaluateDesiredSize();
            update();
        }
    }

    void OverlayTopChatWidget::setPosition(const QPoint& _pos)
    {
        pos_ = _pos;
        update();
    }

    void OverlayTopChatWidget::paintEvent(QPaintEvent* _event)
    {
        if (!text_.isEmpty())
        {
            QPainter p(this);
            const auto badgeWidth = Utils::Badge::getSize(textUnit_->getSourceText().size()).width();
            Utils::Badge::drawBadge(textUnit_, p, pos_.x() + badgeWidth, pos_.y(), Utils::Badge::Color::Green);
        }
    }
}