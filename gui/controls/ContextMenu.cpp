#include "stdafx.h"


#include "ContextMenu.h"

#include "ClickWidget.h"

#include "../fonts.h"
#include "../utils/InterConnector.h"
#include "../main_window/MainWindow.h"

#include "styles/ThemeParameters.h"
#include "styles/StyleSheetGenerator.h"
#include "styles/StyleSheetContainer.h"

#ifdef __APPLE__
#   include "../utils/macos/mac_support.h"
#endif

namespace
{
    auto itemHeight() noexcept { return Utils::scale_value(36); }
    auto fontSize() noexcept { return Utils::scale_value(15);  }
}

namespace Ui
{
    MenuStyle::MenuStyle(const int _iconSize)
        : QProxyStyle()
        , iconSize_(_iconSize)
    {
    }

    int MenuStyle::pixelMetric(PixelMetric _metric, const QStyleOption* _option, const QWidget* _widget) const
    {
        if (_metric == QStyle::PM_SmallIconSize)
            return Utils::scale_value(iconSize_);
        return QProxyStyle::pixelMetric(_metric, _option, _widget);
    }

    TextAction::TextAction(const QString& text, QObject* parent)
        : QWidgetAction(parent)
    {
        textWidget_ = new ClickableTextWidget(nullptr, Fonts::appFont(fontSize()), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, TextRendering::HorAligment::LEFT);
        textWidget_->setFixedHeight(itemHeight());
        textWidget_->setCursor(Qt::ArrowCursor);
        textWidget_->setText(text);
        textWidget_->setLeftPadding(Utils::scale_value(20));
        textWidget_->setHoverColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE));

        QObject::connect(textWidget_, &ClickableTextWidget::clicked, this, &QAction::trigger);
        QObject::connect(textWidget_, &ClickableTextWidget::hoverChanged, this, [this](bool _v)
        {
            if (_v)
            {
                if (auto m = qobject_cast<QMenu*>(parentWidget()))
                    m->setActiveAction(this);
            }
        });

        setDefaultWidget(textWidget_);
    }

    void TextAction::setHovered(bool _v)
    {
        textWidget_->setHovered(_v);
    }

    ContextMenu::ContextMenu(QWidget* _parent, Color _color, int _iconSize)
        : QMenu(_parent)
        , color_(_color)
        , iconSize_(QSize(_iconSize, _iconSize))
    {
        applyStyle(this, true, fontSize(), itemHeight(), _color, iconSize_);
        QObject::connect(this, &QMenu::hovered, this, [this](QAction* action)
        {
            const auto actionList = actions();
            for (auto a : actionList)
            {
                if (a == action)
                {
                    if (auto textAction = qobject_cast<TextAction*>(action))
                    {
                        QSignalBlocker blocker(textAction);
                        textAction->setHovered(true);
                    }
                }
                else if (auto textAction = qobject_cast<TextAction*>(a))
                {
                    textAction->setHovered(false);
                }
            }
        });
    }

    ContextMenu::ContextMenu(QWidget* _parent)
        : ContextMenu(_parent, Color::Default)
    {
    }

    ContextMenu::ContextMenu(QWidget* _parent, int _iconSize)
        : ContextMenu(_parent, Color::Default, _iconSize)
    {
    }

    void ContextMenu::applyStyle(QMenu* _menu, bool _withPadding, int _fonSize, int _height, const QSize& _iconSize)
    {
        applyStyle(_menu, _withPadding, _fonSize, _height, Color::Default, _iconSize);
    }

    void ContextMenu::updatePosition(QMenu* _menu, QPoint _position, bool _forceShowAtLeft)
    {
        const auto focusWin = QGuiApplication::focusWindow();
        if (focusWin == nullptr)
            return;
        auto screenGeometry = focusWin->screen()->geometry();
        const bool macScreenHasEmptyGeometry = platform::is_apple() && screenGeometry.isEmpty();

        if (!_forceShowAtLeft && !macScreenHasEmptyGeometry)
            return;

        auto showAtLeft = _forceShowAtLeft;
        const auto menuWidth = _menu->sizeHint().width();
        const auto menuHeight = _menu->sizeHint().height();
#ifdef __APPLE__
        // Workaround for https://jira.mail.ru/browse/IMDESKTOP-15672
        if (macScreenHasEmptyGeometry)
        {
            screenGeometry = MacSupport::screenGeometryByPoint(_position).toRect();
            showAtLeft |= (_position.x() + menuWidth) > screenGeometry.right();
        }
#endif // __APPLE__

        const auto needExpandUp = (_position.y() + menuHeight) > screenGeometry.bottom();
        _position.rx() -= showAtLeft ? menuWidth : 0;
        _position.ry() -= needExpandUp ? menuHeight : 0;

        if (auto g = _menu->geometry(); g.topLeft() != _position)
        {
            g.moveTopLeft(_position);
            _menu->setGeometry(g);
        }
    }

    void ContextMenu::applyStyle(QMenu* _menu, bool _withPadding, int _fontSize, int _height, Color color, const QSize& _iconSize)
    {
        const auto itemHeight = _height;
        const auto iconPadding = Utils::scale_value((_withPadding ? 20 : 12) + 20);

        auto itemPaddingLeft = Utils::scale_value(_withPadding ? 36 : (platform::is_linux() ? 0 : 20));
        const auto itemPaddingRight = Utils::scale_value(_withPadding ? 28 : 12);

        if (platform::is_linux() && !_withPadding) // fix for linux native menu with icons
            itemPaddingLeft += iconPadding;

        std::unique_ptr<Styling::BaseStyleSheetGenerator> styleString = color == Color::Default
            ? Styling::getParameters().getContextMenuQss(itemHeight, itemPaddingLeft, itemPaddingRight, iconPadding)
            : Styling::getParameters().getContextMenuDarkQss(itemHeight, itemPaddingLeft, itemPaddingRight, iconPadding);
        _menu->setWindowFlags(_menu->windowFlags() | Qt::NoDropShadowWindowHint | Qt::WindowStaysOnTopHint);

        Utils::SetProxyStyle(_menu, new MenuStyle(_iconSize.width()));
        Styling::setStyleSheet(_menu, std::move(styleString));

        _menu->setFont(Fonts::appFont(_fontSize));

        if constexpr (platform::is_apple())
        {
            QObject::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupMenu, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowHide, _menu, &QMenu::close);
            QObject::connect(Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::windowClose, _menu, &QMenu::close);
        }
        else
        {
            QObject::connect(new QShortcut(QKeySequence::Copy, _menu), &QShortcut::activated, Utils::InterConnector::instance().getMainWindow(), &Ui::MainWindow::copy);
        }
    }

    QAction* ContextMenu::addActionWithIcon(const QIcon& _icon, const QString& _name, const QVariant& _data)
    {
        QAction* action = addAction(_icon, _name);
        action->setData(_data);
        return action;
    }

    QAction* ContextMenu::findAction(QStringView _command) const
    {
        im_assert(!_command.isEmpty());
        const auto actionList = actions();
        for (const auto& action : actionList)
        {
            const auto actionParams = action->data().toMap();
            if (const auto commandIter = actionParams.find(qsl("command")); commandIter != actionParams.end())
                if (commandIter->toString() == _command)
                    return action;
        }
        return nullptr;
    }

    QIcon ContextMenu::makeIconForCache(const QString& _iconPath)
    {
        static std::map<QString, QIcon> iconCache;

        static Styling::ThemeChecker checker;
        if (checker.checkAndUpdateHash())
            iconCache.clear();

        auto& icon = iconCache[_iconPath];
        if (icon.isNull())
            icon = makeIcon(_iconPath, iconSize_);

        return icon;
    }

    QColor ContextMenu::normalIconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY);
    }

    QColor ContextMenu::disabledIconColor()
    {
        return Styling::getParameters().getColor(Styling::StyleVariable::BASE_TERTIARY);
    }

    QAction* ContextMenu::addActionWithIcon(const QString& _iconPath, const QString& _name, const QVariant& _data)
    {
        return addActionWithIcon(makeIconForCache(_iconPath), _name, _data);
    }

    bool ContextMenu::modifyAction(QStringView _command, const QString& _iconPath, const QString& _name, const QVariant& _data, bool _enable)
    {
        if (auto action = findAction(_command))
        {
            action->setIcon(makeIconForCache(_iconPath));
            action->setText(_name);
            action->setData(_data);
            action->setEnabled(_enable);
            return true;
        }

        return false;
    }

    bool ContextMenu::hasAction(QStringView _command) const
    {
        return findAction(_command) != nullptr;
    }

    void ContextMenu::removeAction(QStringView _command)
    {
        if (auto action = findAction(_command))
            QWidget::removeAction(action);
    }

    void ContextMenu::showAtLeft(bool _invert)
    {
        needShowAtLeft_ = _invert;
    }

    void ContextMenu::setPosition(const QPoint &_pos)
    {
        pos_ = _pos;
    }

    void ContextMenu::selectBestPositionAroundRectIfNeeded(const QRect& _rect)
    {
        auto screenGeometry = QGuiApplication::focusWindow()->screen()->geometry();
        const bool macScreenHasEmptyGeometry = platform::is_apple() && screenGeometry.isEmpty();

        if (!macScreenHasEmptyGeometry)
            return;

        const auto menuWidth = sizeHint().width();
        const QPoint menuPosAtLeft = { _rect.x(), _rect.y() + _rect.height() / 2 };
        const QPoint menuPosAtRight = { _rect.x() + _rect.width(), _rect.y() + _rect.height() / 2 };
#ifdef __APPLE__
        // Workaround for https://jira.mail.ru/browse/IMDESKTOP-15672
        screenGeometry = MacSupport::screenGeometryByPoint(menuPosAtLeft).toRect();
#endif // __APPLE__
        const bool needShowAtLeft = (menuPosAtRight.x() + menuWidth) > screenGeometry.right();
        setPosition(needShowAtLeft ? menuPosAtLeft : menuPosAtRight);
        showAtLeft(needShowAtLeft);
    }

    void ContextMenu::popup(const QPoint& _pos, QAction* _at)
    {
        setPosition(_pos);
        QMenu::popup(_pos, _at);
    }

    void ContextMenu::clear()
    {
        QMenu::clear();
    }

    void ContextMenu::setWheelCallback(std::function<void(QWheelEvent*)> _callback)
    {
        onWheel_ = std::move(_callback);
    }

    void ContextMenu::setShowAsync(const bool _async)
    {
        showAsync_ = _async;
    }

    bool ContextMenu::isShowAsync() const
    {
        return showAsync_;
    }

    QIcon ContextMenu::makeIcon(const QString& _iconPath, const QSize& _iconSize)
    {
        auto icon = QIcon(Utils::renderSvgScaled(_iconPath, _iconSize, normalIconColor()));
        icon.addPixmap(Utils::renderSvgScaled(_iconPath, _iconSize, disabledIconColor()), QIcon::Mode::Disabled);
        return icon;
    }

    void ContextMenu::showEvent(QShowEvent*)
    {
        const auto p = (pos_ && pos() != pos_) ? *pos_ : pos();
        updatePosition(this, p, needShowAtLeft_);
    }

    void ContextMenu::hideEvent(QHideEvent* _event)
    {
        QMenu::hideEvent(_event);
        Q_EMIT hidden(QPrivateSignal());
    }

    void ContextMenu::focusOutEvent(QFocusEvent *_e)
    {
        if (parentWidget() && !parentWidget()->isActiveWindow())
        {
            close();
            return;
        }
        QMenu::focusOutEvent(_e);
    }

    void ContextMenu::wheelEvent(QWheelEvent* _event)
    {
        if (onWheel_)
            onWheel_(_event);
        QMenu::wheelEvent(_event);
    }

    void ContextMenu::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (!rect().contains(_event->pos()) && isShowAsync())
            close();
        else
            QMenu::mouseReleaseEvent(_event);
    }
}
