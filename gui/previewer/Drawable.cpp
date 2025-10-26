#include "Drawable.h"
#include "../utils/utils.h"

using namespace Ui;


void Button::draw(QPainter &_p)
{
    if (!visible_)
        return;

    QPixmap pixmap;

    if (disabled_)
        pixmap = disabledPixmap_;
    else if (checked_)
        pixmap = checkedPixmap_;
    else if (pressed_)
        pixmap = pressedPixmap_;
    else if (hovered_)
        pixmap = hoveredPixmap_;

    if (pixmap.isNull())
        pixmap = defaultPixmap_;

    if (pixmap.isNull())
        return;

    QRect rect;
    const auto ratio = pixmap.devicePixelRatio();

    if (pixmapCentered_)
        rect = QRect(rect_.x() + (rect_.width() - pixmap.width() / ratio) / 2,
                     rect_.y() + (rect_.height() - pixmap.height() / ratio) / 2,
                     pixmap.width() / ratio, pixmap.height() / ratio);
    else
        rect = QRect(rect_.x() + offset_.x, rect_.y() + offset_.y, pixmap.width() / ratio, pixmap.height() / ratio);

//    _p.save();
//    _p.setPen(Qt::cyan);
//    _p.drawRect(rect);
//    _p.restore();

    _p.drawPixmap(rect, pixmap);
}

void Button::setDefaultPixmap(const QPixmap &_pixmap)
{
    defaultPixmap_ = _pixmap;
    Utils::check_pixel_ratio(defaultPixmap_);
}

void Button::setHoveredPixmap(const QPixmap &_pixmap)
{
    hoveredPixmap_ = _pixmap;
    Utils::check_pixel_ratio(hoveredPixmap_);
}

void Button::setPressedPixmap(const QPixmap &_pixmap)
{
   pressedPixmap_ = _pixmap;
   Utils::check_pixel_ratio(pressedPixmap_);
}

void Button::setDisabledPixmap(const QPixmap &_pixmap)
{
    disabledPixmap_ = _pixmap;
    Utils::check_pixel_ratio(disabledPixmap_);
}

void Button::setCheckedPixmap(const QPixmap &_pixmap)
{
    checkedPixmap_ = _pixmap;
    Utils::check_pixel_ratio(checkedPixmap_);
}

void Label::draw(QPainter &_p)
{
    if (textUnit_ && visible_)
        textUnit_->draw(_p, pos_);

//    _p.save();
//    _p.setPen(Qt::green);
//    _p.drawRect(rect_);
//    _p.restore();
}

void Label::setHovered(bool _hovered)
{
    Drawable::setHovered(_hovered);

    updateColor();
}

void Label::setPressed(bool _pressed)
{
    Drawable::setPressed(_pressed);

    updateColor();
}

void Label::setDisabled(bool _disabled)
{
    Drawable::setDisabled(_disabled);

    updateColor();
}

void Label::setRect(const QRect &_rect)
{
    Drawable::setRect(_rect);

    if (!textUnit_)
        return;

    textUnit_->getHeight(_rect.width());
    textUnit_->setOffsets(_rect.x() + offset_.x, _rect.y() + offset_.y);
}

void Label::setText(const QString &_text)
{
    if (!textUnit_)
        return;

    textUnit_->setText(_text);
    textUnit_->getHeight(rect_.width());
}

void Ui::Label::initTextUnit(const TextRendering::TextUnit::InitializeParameters& _params)
{
    if (textUnit_)
    {
        textUnit_->init(_params);
        textUnit_->getHeight(textUnit_->desiredWidth());
    }
}

void Label::setDefaultColor(const Styling::ColorParameter& _color)
{
    defaultColor_ = _color;
    updateColor();
}

void Label::setUnderline(bool _enable)
{
    if (textUnit_)
        textUnit_->setUnderline(_enable);
}

int Label::desiredWidth()
{
    if (textUnit_)
        return textUnit_->desiredWidth();
    else
        return 0;
}

int Label::getHeight(int _width)
{
    if (textUnit_)
        return textUnit_->getHeight(_width);
    else
        return 0;
}

void Label::updateColor()
{
    if (!textUnit_)
        return;

    Styling::ColorParameter parameter = defaultColor_;

    if (disabled_ && disabledColor_.isValid())
        parameter = disabledColor_;
    else if (pressed_ && pressedColor_.isValid())
        parameter = pressedColor_;
    else if (hovered_ && hoveredColor_.isValid())
        parameter = hoveredColor_;

    if (parameter.isValid())
        textUnit_->setColor(parameter);
}

QString Label::getText() const
{
    return textUnit_->getText();
}

void BDrawable::draw(QPainter &_p)
{
    if (!visible_)
        return;

    QPainterPath path;

    path.addRoundedRect(rect_, borderRadius_, borderRadius_);

    if (pressed_ && pressedBackground_.isValid())
        _p.fillPath(path, pressedBackground_.actualColor());
    else if (hovered_ && hoveredBackground_.isValid())
        _p.fillPath(path, hoveredBackground_.actualColor());
    else if (background_.isValid())
        _p.fillPath(path, background_.actualColor());
}

void BButton::draw(QPainter &_p)
{
    BDrawable::draw(_p);
    Button::draw(_p);
}

void Icon::draw(QPainter &_p)
{
    const auto ratio = pixmap_.devicePixelRatio();
    auto rect = QRect(rect_.x() + offset_.x, rect_.y() + offset_.y, pixmap_.width() / ratio, pixmap_.height() / ratio);
    _p.drawPixmap(rect, pixmap_);
}

void Icon::setPixmap(const QPixmap &_pixmap)
{
    pixmap_ = _pixmap;
    Utils::check_pixel_ratio(pixmap_);
}

void BLabel::draw(QPainter &_p)
{
    BDrawable::draw(_p);
    Label::draw(_p);
}

QAccessibleInterface* AccessibleButton::parent() const
{
    auto result = QAccessible::queryAccessibleInterface(object()->parent());
    im_assert(result);
    return result;
}

QAccessible::State AccessibleButton::state() const
{
    QAccessible::State state;
    auto button = qobject_cast<ButtonAccessible*>(object());
    im_assert(button);
    if (button && !button->clickable())
        state.disabled = 1;
    return state;
}

QRect Ui::AccessibleButton::rect() const
{
    auto button = qobject_cast<Ui::ButtonAccessible*>(object());
    im_assert(button);
    if (button)
    {
        auto parentWidget = qobject_cast<QWidget*>(button->parent());
        im_assert(parentWidget);
        if (parentWidget)
            return QRect(parentWidget->mapToGlobal(button->rect().topLeft()), button->rect().size());
    }
    return QRect();
}

QString AccessibleButton::text(QAccessible::Text t) const
{
    QString result;
    if (QAccessible::Text::Name == t)
        result = qobject_cast<ButtonAccessible*>(object())->accessibleName();
    return result;
}
