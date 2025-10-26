#include "stdafx.h"

#include "SwitcherCheckbox.h"
#include "../utils/utils.h"
#include "../styles/ThemeParameters.h"

namespace
{
    QSize getSize() noexcept
    {
        return Utils::scale_value(QSize(36, 20));
    }

    QPixmap getOnIcon()
    {
        static auto pm = Utils::StyledPixmap(qsl(":/controls/switch_on_icon"), getSize(), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
        return pm.actualPixmap();
    }

    QPixmap getOffIcon()
    {
        static auto pm = Utils::StyledPixmap(qsl(":/controls/switch_off_icon"), getSize(), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_TERTIARY });
        return pm.actualPixmap();
    }

    constexpr double disabledAlpha() noexcept
    {
        return 0.4;
    }
}

namespace Ui
{
    SwitcherCheckbox::SwitcherCheckbox(QWidget* _parent)
        : QCheckBox(_parent)
    {
        setFixedSize(getSize());
        setCursor(Qt::PointingHandCursor);

        connect(this, &SwitcherCheckbox::toggled, this, qOverload<>(&SwitcherCheckbox::update));
    }

    void SwitcherCheckbox::paintEvent(QPaintEvent*)
    {
        const auto pm = isChecked() ? getOnIcon() : getOffIcon();
        const auto x = (width() - pm.width() / Utils::scale_bitmap_ratio()) / 2;
        const auto y = (height() - pm.height() / Utils::scale_bitmap_ratio()) / 2;

        QPainter p(this);

        if (!isEnabled())
            p.setOpacity(disabledAlpha());

        p.drawPixmap(x, y, pm);
    }

    bool SwitcherCheckbox::hitButton(const QPoint&) const
    {
        return true;
    }

}