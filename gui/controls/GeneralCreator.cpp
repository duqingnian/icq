#include "stdafx.h"
#include "GeneralCreator.h"

#include "FlatMenu.h"
#include "TextEmojiWidget.h"
#include "SwitcherCheckbox.h"
#include "CustomButton.h"
#include "../core_dispatcher.h"
#include "../utils/utils.h"
#include "textrendering/TextRenderingUtils.h"
#include "../styles/ThemeParameters.h"
#include "../styles/StyleSheetContainer.h"
#include "../styles/StyleSheetGenerator.h"
#include "main_window/sidebar/SidebarUtils.h"

namespace
{
    auto sliderHandleWidth() noexcept
    {
        return Utils::scale_value(8);
    }

    auto sliderHandleHeight() noexcept
    {
        return Utils::scale_value(24);
    }

    QSize sliderHandleSize() noexcept
    {
        return { sliderHandleWidth(), sliderHandleHeight() };
    }

    QString sliderHandleIconPath()
    {
        return qsl(":/controls/selectbar");
    }

}

class SliderProxyStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption* option = 0, const QWidget* widget = 0) const override
    {
        switch (metric)
        {
        case PM_SliderLength:
            return sliderHandleWidth();
        case PM_SliderThickness:
            return sliderHandleHeight();
        default:
            return (QProxyStyle::pixelMetric(metric, option, widget));
        }
    }
};


class ComboboxProxyStyle : public QProxyStyle
{
public:
    int pixelMetric(PixelMetric metric, const QStyleOption * option = 0, const QWidget * widget = 0) const override
    {
        switch (metric)
        {
            case PM_SmallIconSize: return Utils::scale_value(24);
            default: return (QProxyStyle::pixelMetric(metric, option, widget));
        }
    }
};

namespace Ui
{
    void GeneralCreator::addHeader(QWidget* _parent, QLayout* _layout, const QString& _text, const int _leftMargin)
    {
        auto mainWidget = new QWidget(_parent);
        auto layout = Utils::emptyHLayout(mainWidget);
        if (_leftMargin)
            layout->addSpacing(Utils::scale_value(_leftMargin));
        auto title = new TextEmojiWidget(
            _parent,
            Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold),
            Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });

        title->setText(_text);
        Testing::setAccessibleName(title, u"AS GeneralCreator" % _text);
        layout->addWidget(title);
        _layout->addWidget(mainWidget);
        Utils::grabTouchWidget(title);
    }

    QWidget* GeneralCreator::addHotkeyInfo(QWidget* _parent, QLayout* _layout, const QString& _name, const QString& _keys)
    {
        auto infoWidget = new QWidget(_parent);
        auto infoLayout = Utils::emptyHLayout(infoWidget);
        infoLayout->setAlignment(Qt::AlignTop);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(Utils::scale_value(12));

        auto textName = new TextEmojiWidget(infoWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        auto textKeys = new TextEmojiWidget(infoWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY });

        textName->setText(_name);
        textName->setMultiline(true);
        textKeys->setText(_keys, TextEmojiAlign::allign_right);

        QSizePolicy sp(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Expanding);
        sp.setHorizontalStretch(1);
        textName->setSizePolicy(sp);
        textKeys->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Fixed);

        infoLayout->addWidget(textName);
        infoLayout->addStretch();
        infoLayout->addWidget(textKeys);

        Testing::setAccessibleName(infoWidget, u"AS GeneralCreator hotkeyInfoWidget" % _keys);
        Utils::grabTouchWidget(infoWidget);
        _layout->addWidget(infoWidget);
        return infoWidget;
    }

    SidebarCheckboxButton* GeneralCreator::addSwitcher(QWidget *_parent, QLayout *_layout, const QString &_text,
                                                       bool _switched, std::function<void(bool)> _slot, int _height, const QString& _accName)
    {
        const auto switcherRow = addCheckbox(QString(), _text, _parent, _layout);
        switcherRow->setFixedHeight(_height == -1 ? Utils::scale_value(44) : _height);
        switcherRow->setChecked(_switched);
        switcherRow->setTextOffset(0);
        switcherRow->setContentsMargins(0, 0, 0, 0);
        switcherRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        Testing::setAccessibleName(switcherRow, _accName);

        QObject::connect(switcherRow, &SidebarCheckboxButton::clicked, switcherRow, [switcherRow, slot = std::move(_slot)]()
        {
            if (slot)
                slot(switcherRow->isChecked());
        });

        return switcherRow;
    }

    TextEmojiWidget* GeneralCreator::addChooser(QWidget* _parent, QLayout* _layout, const QString& _info, const QString& _value, std::function< void(TextEmojiWidget*) > _slot, const QString& _accName)
    {
        auto mainWidget = new QWidget(_parent);

        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->addSpacing(Utils::scale_value(20));
        mainLayout->setAlignment(Qt::AlignLeft);
        mainLayout->setSpacing(Utils::scale_value(4));

        Utils::grabTouchWidget(mainWidget);

        auto info = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        Utils::grabTouchWidget(info);
        info->setSizePolicy(QSizePolicy::Policy::Preferred, info->sizePolicy().verticalPolicy());
        info->setText(_info);
        Testing::setAccessibleName(info, u"AS GeneralCreator " % _info);
        mainLayout->addWidget(info);

        auto valueWidget = new QWidget(_parent);
        auto valueLayout = Utils::emptyVLayout(valueWidget);
        Utils::grabTouchWidget(valueWidget);
        valueWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        valueLayout->setAlignment(Qt::AlignBottom);
        valueLayout->setContentsMargins(0, 0, 0, 0);

        auto value = new TextEmojiWidget(valueWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
        {
            Utils::grabTouchWidget(value);
            value->setCursor(Qt::PointingHandCursor);
            value->setText(_value);
            value->setEllipsis(true);
            value->setHoverColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_HOVER });
            value->setActiveColor(Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY_ACTIVE });
            QObject::connect(value, &TextEmojiWidget::clicked, value, [value, slot = std::move(_slot)]()
            {
                if (slot)
                    slot(value);
            });
            Testing::setAccessibleName(value, _accName);
            valueLayout->addWidget(value);
        }
        Testing::setAccessibleName(valueWidget, qsl("AS GeneralCreator valueWidget"));
        mainLayout->addWidget(valueWidget);

        Testing::setAccessibleName(mainWidget, qsl("AS GeneralCreator mainWidget"));
        _layout->addWidget(mainWidget);
        return value;
    }

    GeneralCreator::DropperInfo GeneralCreator::addDropper(QWidget* _parent, QLayout* _layout, const QString& _info, bool _isHeader, const std::vector< QString >& _values, int _selected, int _width, std::function< void(const QString&, int, TextEmojiWidget*) > _slot)
    {
        TextEmojiWidget* title = nullptr;
        TextEmojiWidget* aw1 = nullptr;
        auto titleWidget = new QWidget(_parent);
        auto titleLayout = Utils::emptyHLayout(titleWidget);
        Utils::grabTouchWidget(titleWidget);
        titleWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        titleLayout->setAlignment(Qt::AlignLeft);
        {
            const QFont font = _isHeader ? Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold) : Fonts::appFontScaled(15);

            title = new TextEmojiWidget(titleWidget, font, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            Utils::grabTouchWidget(title);
            title->setSizePolicy(QSizePolicy::Policy::Preferred, title->sizePolicy().verticalPolicy());
            title->setText(_info);
            Testing::setAccessibleName(title, qsl("AS GeneralCreator title"));
            titleLayout->addWidget(title);

            aw1 = new TextEmojiWidget(titleWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            Utils::grabTouchWidget(aw1);
            aw1->setSizePolicy(QSizePolicy::Policy::Preferred, aw1->sizePolicy().verticalPolicy());
            aw1->setText(TextRendering::spaceAsString());
            Testing::setAccessibleName(aw1, qsl("AS GeneralCreator something"));
            titleLayout->addWidget(aw1);
        }
        Testing::setAccessibleName(titleWidget, qsl("AS GeneralCreator titleWidget"));
        _layout->addWidget(titleWidget);

        auto selectedItemWidget = new QWidget(_parent);
        auto selectedItemLayout = Utils::emptyHLayout(selectedItemWidget);
        Utils::grabTouchWidget(selectedItemWidget);
        selectedItemWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
        selectedItemWidget->setFixedHeight(Utils::scale_value(32));
        selectedItemLayout->setAlignment(Qt::AlignLeft);

        DropperInfo di;
        di.currentSelected = nullptr;
        di.menu = nullptr;
        {
            auto selectedItemButton = new QPushButton(selectedItemWidget);
            auto dl = Utils::emptyVLayout(selectedItemButton);
            selectedItemButton->setFlat(true);
            selectedItemButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
            auto selectedWidth = _width == -1 ? 280 : _width;
            selectedItemButton->setFixedSize(Utils::scale_value(QSize(selectedWidth, 32)));
            selectedItemButton->setCursor(Qt::CursorShape::PointingHandCursor);
            selectedItemButton->setStyleSheet(qsl("border: none; outline: none;"));
            {
                auto selectedItem = new TextEmojiWidget(selectedItemButton, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY }, Utils::scale_value(24));
                Utils::grabTouchWidget(selectedItem);

                auto selectedItemText = _selected < int(_values.size()) ? _values[_selected] : TextRendering::spaceAsString();
                selectedItem->setText(selectedItemText);
                selectedItem->setFading(true);
                Testing::setAccessibleName(selectedItem, u"AS GeneralCreator selectedItem " % selectedItemText);
                dl->addWidget(selectedItem);
                di.currentSelected = selectedItem;

                auto lp = new QWidget(selectedItemButton);
                auto lpl = Utils::emptyHLayout(lp);
                Utils::grabTouchWidget(lp);
                lpl->setAlignment(Qt::AlignBottom);
                {
                    auto ln = new QFrame(lp);
                    Utils::grabTouchWidget(ln);
                    ln->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    ln->setFrameShape(QFrame::StyledPanel);
                    Utils::ApplyStyle(ln, ql1s("border-width: 1dip; border-bottom-style: solid; border-color: %1; ").arg(Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY)));//DROPDOWN_STYLE);
                    Testing::setAccessibleName(ln, qsl("AS GeneralCreator ln"));
                    lpl->addWidget(ln);
                }
                Testing::setAccessibleName(lp, qsl("AS GeneralCreator lp"));
                dl->addWidget(lp);

                auto menu = new FlatMenu(selectedItemButton);
                menu->setMinimumWidth(Utils::scale_value(selectedWidth));
                menu->setExpandDirection(Qt::AlignBottom);
                menu->setNeedAdjustSize(true);

                Testing::setAccessibleName(menu, qsl("AS GeneralCreator flatMenu"));
                for (const auto& v : _values)
                {
                    auto a = menu->addAction(v);
                    a->setProperty(FlatMenu::sourceTextPropName(), v);
                }
                QObject::connect(menu, &QMenu::triggered, _parent, [menu, aw1, selectedItem, slot = std::move(_slot)](QAction* a)
                {
                    int ix = -1;
                    const QList<QAction*> allActions = menu->actions();
                    for (QAction* action : allActions)
                    {
                        ++ix;
                        if (a == action)
                        {
                            const auto text = a->property(FlatMenu::sourceTextPropName()).toString();
                            selectedItem->setText(text);
                            if (slot)
                                slot(text, ix, aw1);
                            break;
                        }
                    }
                });
                selectedItemButton->setMenu(menu);
                di.menu = menu;
            }
            Testing::setAccessibleName(selectedItemButton, qsl("AS GeneralCreator selectedItemButton"));
            selectedItemLayout->addWidget(selectedItemButton);
        }
        Testing::setAccessibleName(selectedItemWidget, qsl("AS GeneralCreator selectedItemWidget"));
        _layout->addWidget(selectedItemWidget);

        return di;
    }

    GeneralCreator::ProgresserDescriptor GeneralCreator::addProgresser(
        QWidget* _parent,
        QLayout* _layout,
        int _markCount,
        int _selected,
        std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slotFinish,
        std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > _slotProgress)
    {
        TextEmojiWidget* w = nullptr;
        TextEmojiWidget* aw = nullptr;
        auto mainWidget = new QWidget(_parent);
        Utils::grabTouchWidget(mainWidget);
        auto mainLayout = Utils::emptyHLayout(mainWidget);
        mainLayout->addSpacing(Utils::scale_value(20));
        mainWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        mainLayout->setAlignment(Qt::AlignLeft);
        mainLayout->setSpacing(Utils::scale_value(4));
        {
            w = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            Utils::grabTouchWidget(w);
            w->setSizePolicy(QSizePolicy::Policy::Preferred, w->sizePolicy().verticalPolicy());
            w->setText(TextRendering::spaceAsString());
            Testing::setAccessibleName(w, qsl("AS GeneralCreator w"));
            mainLayout->addWidget(w);

            aw = new TextEmojiWidget(mainWidget, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            Utils::grabTouchWidget(aw);
            aw->setSizePolicy(QSizePolicy::Policy::Preferred, aw->sizePolicy().verticalPolicy());
            aw->setText(TextRendering::spaceAsString());
            Testing::setAccessibleName(aw, qsl("AS GeneralCreator aw"));
            mainLayout->addWidget(aw);

            _slotFinish(w, aw, _selected);
        }
        Testing::setAccessibleName(mainWidget, qsl("AS GeneralCreator mainWidget"));
        _layout->addWidget(mainWidget);

        auto sliderWidget = new QWidget(_parent);
        Utils::grabTouchWidget(sliderWidget);
        auto sliderLayout = Utils::emptyVLayout(sliderWidget);
        sliderWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        sliderWidget->setFixedWidth(Utils::scale_value(300));
        sliderLayout->setAlignment(Qt::AlignBottom);
        sliderLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(12), 0, 0);
        {
            auto slider = new SettingsSlider(Qt::Orientation::Horizontal, _parent);
            Utils::grabTouchWidget(slider);
            slider->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
            slider->setFixedWidth(Utils::scale_value(280));
            slider->setFixedHeight(Utils::scale_value(24));
            slider->setMinimum(0);
            slider->setMaximum(_markCount);
            slider->setValue(_selected);
            slider->setCursor(QCursor(Qt::CursorShape::PointingHandCursor));

            QObject::connect(slider, &QSlider::sliderReleased, [w, aw, _slotFinish, slider]()
            {

                _slotFinish(w, aw, slider->value());
                w->update();
                aw->update();
            });

            QObject::connect(slider, &QSlider::valueChanged, [w, aw, _slotProgress, slider]()
            {

                _slotProgress(w, aw, slider->value());
                w->update();
                aw->update();
            });

            Testing::setAccessibleName(slider, qsl("AS GeneralCreator slider"));
            sliderLayout->addWidget(slider);
        }
        Testing::setAccessibleName(sliderWidget, qsl("AS GeneralCreator sliderWidget"));
        _layout->addWidget(sliderWidget);

        return { mainWidget, sliderWidget };
    }

    void GeneralCreator::addBackButton(QWidget* _parent, QLayout* _layout, std::function<void()> _onButtonClick)
    {
        auto backArea = new QWidget(_parent);
        backArea->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding));
        auto backLayout = new QVBoxLayout(backArea);
        backLayout->setContentsMargins(Utils::scale_value(24), Utils::scale_value(24), 0, 0);

        auto backButton = new CustomButton(backArea, qsl(":/controls/back_icon"), QSize(20, 20));
        Styling::Buttons::setButtonDefaultColors(backButton);
        backButton->setFixedSize(Utils::scale_value(QSize(24, 24)));
        Testing::setAccessibleName(backButton, qsl("AS GeneralCreator backButton"));
        backLayout->addWidget(backButton);

        auto verticalSpacer = new QSpacerItem(Utils::scale_value(15), Utils::scale_value(543), QSizePolicy::Minimum, QSizePolicy::Expanding);
        backLayout->addItem(verticalSpacer);

        Testing::setAccessibleName(backArea, qsl("AS GeneralCreator backArea"));
        _layout->addWidget(backArea);

        if (_onButtonClick)
            QObject::connect(backButton, &QPushButton::clicked, _onButtonClick);
    }

    QComboBox* GeneralCreator::addComboBox(QWidget * _parent, QLayout * _layout, const QString & _info, bool _isHeader, const std::vector<QString>& _values, int _selected, int _width, std::function<void(const QString&, int)> _slot)
    {
        if (!_info.isEmpty())
        {
            TextEmojiWidget* title = nullptr;
            auto titleWidget = new QWidget(_parent);
            auto titleLayout = Utils::emptyHLayout(titleWidget);
            Utils::grabTouchWidget(titleWidget);
            titleWidget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            titleLayout->setAlignment(Qt::AlignLeft);

            QFont font;
            if (!_isHeader)
                font = Fonts::appFontScaled(15);
            else
                font = Fonts::appFontScaled(17, Fonts::FontWeight::SemiBold);

            title = new TextEmojiWidget(titleWidget, font, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
            Utils::grabTouchWidget(title);
            title->setSizePolicy(QSizePolicy::Policy::Preferred, title->sizePolicy().verticalPolicy());
            title->setText(_info);
            Testing::setAccessibleName(title, u"AS GeneralCreator " % _info);
            titleLayout->addWidget(title);

            Testing::setAccessibleName(titleWidget, qsl("AS GeneralCreator titleWidget"));
            _layout->addWidget(titleWidget);
        }

        auto cmbBox = new QComboBox(_parent);
        Utils::SetProxyStyle(cmbBox, new ComboboxProxyStyle());
        Styling::setStyleSheet(cmbBox, Styling::getParameters().getComboBoxQss());
        cmbBox->setFont(Fonts::appFontScaled(16));
        cmbBox->setMaxVisibleItems(6);
        cmbBox->setCursor(Qt::PointingHandCursor);
        cmbBox->setFixedWidth(Utils::scale_value(280));
        Testing::setAccessibleName(cmbBox, qsl("AS GeneralCreator cmbBoxProblems"));
        Testing::setAccessibleName(cmbBox->view(), qsl("AS GeneralCreator flatMenu"));

        QPixmap pixmap(1, Utils::scale_value(50));
        pixmap.fill(Qt::transparent);
        QIcon icon(pixmap);
        cmbBox->setIconSize(QSize(1, Utils::scale_value(50)));

        auto metrics = cmbBox->fontMetrics();
        auto viewWidth = 0;
        for (const auto& v : _values)
        {
            viewWidth = std::max(viewWidth, metrics.horizontalAdvance(v));
            cmbBox->addItem(icon, v);
        }

        const auto maxViewWidth = Utils::scale_value(400);
        viewWidth = std::max(viewWidth + Utils::scale_value(32), cmbBox->width());

        cmbBox->view()->setFixedWidth(std::min(maxViewWidth, viewWidth));
        if (_slot)
            QObject::connect(cmbBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), _parent, [cmbBox, slot = std::move(_slot)](int _idx) { slot(cmbBox->itemText(_idx), _idx); });

        _layout->addWidget(cmbBox);
        return cmbBox;
    }

    SettingsSlider::SettingsSlider(Qt::Orientation _orientation, QWidget* _parent)
        : QSlider(_orientation, _parent)
        , handleNormal_ { Utils::StyledPixmap(sliderHandleIconPath(), sliderHandleSize(), Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY }) }
        , handleHovered_ { Utils::StyledPixmap(sliderHandleIconPath(), sliderHandleSize(), Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_HOVER }) }
        , handlePressed_ { Utils::StyledPixmap(sliderHandleIconPath(), sliderHandleSize(), Styling::ThemeColorKey { Styling::StyleVariable::PRIMARY_ACTIVE }) }
        , handleDisabled_ { Utils::StyledPixmap(sliderHandleIconPath(), sliderHandleSize(), Styling::ThemeColorKey { Styling::StyleVariable::BASE_SECONDARY }) }
        , hovered_(false)
        , pressed_(false)
    {
        Utils::SetProxyStyle(this, new SliderProxyStyle());
    }

    SettingsSlider::~SettingsSlider() = default;

    bool SettingsSlider::setPosition(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton)
        {
            setValue(QStyle::sliderValueFromPosition(minimum(), maximum(),
                (orientation() == Qt::Vertical) ? _event->y() : _event->x(),
                (orientation() == Qt::Vertical) ? height() : width()));

            _event->accept();

            return true;
        }
        return false;
    }

    void SettingsSlider::mousePressEvent(QMouseEvent * _event)
    {
        QSlider::mousePressEvent(_event);
        setPosition(_event);
    }

    void SettingsSlider::mouseReleaseEvent(QMouseEvent* _event)
    {
        QSlider::mouseReleaseEvent(_event);

        if (setPosition(_event))
            Q_EMIT sliderReleased();
    }

    void SettingsSlider::enterEvent(QEvent * _event)
    {
        hovered_ = true;
        QSlider::enterEvent(_event);
    }

    void SettingsSlider::leaveEvent(QEvent * _event)
    {
        hovered_ = false;
        QSlider::leaveEvent(_event);
    }

    void SettingsSlider::wheelEvent(QWheelEvent* _e)
    {
        // Disable mouse wheel event for sliders
        if (parent())
            parent()->event(_e);
    }

    void SettingsSlider::paintEvent(QPaintEvent * _event)
    {
        QPainter painter(this);
        QStyleOptionSlider opt;
        initStyleOption(&opt);

        QRect handleRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        handleRect.setSize(QSize(Utils::scale_value(8), Utils::scale_value(24)));
        handleRect.setTop(0);

        QRect grooveOrigRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
        QRect grooveRect = grooveOrigRect;

        grooveRect.setHeight(Utils::scale_value(2));
        grooveRect.moveCenter(grooveOrigRect.center());

        painter.fillRect(grooveRect, Styling::getParameters().getColor(Styling::StyleVariable::BASE_SECONDARY));
        if (isEnabled())
            painter.drawPixmap(handleRect, hovered_ ? handleHovered_.actualPixmap() : (pressed_ ? handlePressed_.actualPixmap() : handleNormal_.actualPixmap()));
        else
            painter.drawPixmap(handleRect, handleDisabled_.actualPixmap());
    }
}
