#pragma once

#include "../controls/TextUnit.h"

namespace Ui
{

class Drawable
{
public:

    virtual ~Drawable() = default;

    QRect rect() const { return rect_;}
    virtual void setRect(const QRect& _rect) { rect_ = _rect; }

    virtual void draw(QPainter& _p) {}

    virtual void setHovered(bool _hovered) { hovered_ = _hovered; }
    virtual void setPressed(bool _pressed) { pressed_ = _pressed; }
    virtual void setDisabled(bool _disabled) { disabled_ = _disabled; }
    virtual void setVisible(bool _visible) { visible_ = _visible; }
    virtual void setChecked(bool _checked) { checked_ = _checked; }

    virtual void setCheckable(bool _checkable) { checkable_ = _checkable; }

    virtual bool hovered() const { return hovered_; }
    virtual bool pressed() const { return pressed_; }
    virtual bool disabled() const { return disabled_; }
    virtual bool visible() const { return visible_; }
    virtual bool checked() const { return checked_; }

    virtual bool clickable() const { return false; }
    virtual bool checkable() const { return checkable_; }

    virtual void setXOffset(int _x) { offset_.x = _x; }
    virtual void setYOffset(int _y) { offset_.y = _y; }


protected:
    QRect rect_;
    bool hovered_ = false;
    bool pressed_ = false;
    bool disabled_ = false;
    bool checked_ = false;
    bool visible_ = true;
    bool checkable_ = false;
    struct {int x, y;} offset_ = {0, 0};
};

class Button : virtual public Drawable
{
public:
    virtual ~Button() = default;

    void draw(QPainter& _p) override;

    bool clickable() const override { return visible_ && !disabled_; }

    void setDefaultPixmap(const QPixmap& _pixmap);
    void setHoveredPixmap(const QPixmap& _pixmap);
    void setPressedPixmap(const QPixmap& _pixmap);
    void setDisabledPixmap(const QPixmap& _pixmap);
    void setCheckedPixmap(const QPixmap& _pixmap);

    void setPixmapCentered(bool _centered) { pixmapCentered_ = _centered; }

protected:
    bool pixmapCentered_ = false;
    QPixmap defaultPixmap_;
    QPixmap hoveredPixmap_;
    QPixmap pressedPixmap_;
    QPixmap disabledPixmap_;
    QPixmap checkedPixmap_;
};

class ButtonAccessible : public QObject, public Button
{
    Q_OBJECT

Q_SIGNALS:
    void clicked() const;

public:
    ButtonAccessible(QObject* parent) : QObject(parent) { }
    void setAccessibleName(const QString& name) { accessibleName_ = name; }
    const QString& accessibleName() const { return accessibleName_; }

protected:
    QString accessibleName_;
};

class AccessibleButton : public QAccessibleObject
{
public:
    AccessibleButton(ButtonAccessible* button) : QAccessibleObject(button) { im_assert(button); }

    int childCount() const override { return 0; }

    QAccessibleInterface* child(int index) const override { return nullptr; }

    QAccessibleInterface* parent() const override;

    QAccessible::State state() const override;

    QAccessible::Role role() const override { return QAccessible::Role::Button; }

    QRect rect() const override;

    int indexOfChild(const QAccessibleInterface* child) const override { return -1; }

    QString text(QAccessible::Text t) const override;
};

class Label : virtual public Drawable
{
public:
    virtual ~Label() = default;
    void draw(QPainter& _p) override;

    bool clickable() const override { return clickable_ && visible_ && !disabled_; }

    void setHovered(bool _hovered) override;
    void setPressed(bool _pressed) override;
    void setDisabled(bool _disabled) override;

    void setRect(const QRect& _rect) override;
    void setText(const QString& _text);
    void setClickable(bool _clickable) { clickable_ = _clickable; }

    void setTextUnit(TextRendering::TextUnitPtr _textUnit) { textUnit_ = std::move(_textUnit); }

    void initTextUnit(const TextRendering::TextUnit::InitializeParameters& _params);

    void setVerticalPosition(TextRendering::VerPosition _pos) { pos_ = _pos; }

    void setDefaultColor(const Styling::ColorParameter& _color);
    void setHoveredColor(const Styling::ColorParameter& _color) { hoveredColor_ = _color; }
    void setPressedColor(const Styling::ColorParameter& _color) { pressedColor_ = _color; }
    void setDisabledColor(const Styling::ColorParameter& _color) { disabledColor_ = _color; }

    void setUnderline(bool _enable);
    int desiredWidth();
    int getHeight(int _width);
    QString getText() const;

protected:
    void updateColor();

    Styling::ColorParameter defaultColor_;
    Styling::ColorParameter hoveredColor_;
    Styling::ColorParameter pressedColor_;
    Styling::ColorParameter disabledColor_;
    TextRendering::TextUnitPtr textUnit_;
    TextRendering::VerPosition pos_ = TextRendering::VerPosition::TOP;
    bool clickable_ = false;
};

class BDrawable : virtual public Drawable // Drawable with background
{
public:
    virtual ~BDrawable() = default;
    void draw(QPainter& _p) override;

    virtual void setBorderRadius(int _radius) { borderRadius_ = _radius; }

    Styling::ColorContainer background_;
    Styling::ColorContainer hoveredBackground_;
    Styling::ColorContainer pressedBackground_;

    int borderRadius_ = 0;
};

class BButton : public Button, public BDrawable
{
public:
    virtual ~BButton() = default;
    void draw(QPainter& _p) override;
};

class Icon : virtual public Drawable
{
public:
    virtual ~Icon() = default;
    void draw(QPainter& _p) override;
    void setPixmap(const QPixmap& _pixmap);
    QSize pixmapSize() { return pixmap_.size() / pixmap_.devicePixelRatio(); }
    bool isNullPixmap() { return pixmap_.isNull(); }

protected:
    QPixmap pixmap_;
};

class BLabel : public Label, public BDrawable // Label with background
{
public:
    virtual ~BLabel() = default;
    void draw(QPainter& _p) override;
};

}
