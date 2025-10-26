#include "stdafx.h"

#include "PanelDisabled.h"

#include "controls/LabelEx.h"
#include "utils/utils.h"
#include "styles/ThemeParameters.h"
#include "fonts.h"

namespace Ui
{
    InputPanelDisabled::InputPanelDisabled(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(getDefaultInputHeight());

        label_ = new LabelEx(this);
        label_->setFont(Fonts::appFontScaled(14));
        label_->setAlignment(Qt::AlignCenter);
        label_->setWordWrap(true);
        label_->setTextInteractionFlags(Qt::NoTextInteraction);
        Testing::setAccessibleName(label_, qsl("AS ChatInput disabledInput"));

        auto l = Utils::emptyVLayout(this);
        l->addWidget(label_);

        setState(DisabledPanelState::Empty);
        updateLabelColor();
    }

    void InputPanelDisabled::setState(const DisabledPanelState _state)
    {
        switch (_state)
        {
        case DisabledPanelState::Banned:
            label_->setText(QT_TRANSLATE_NOOP("input_widget", "You was banned to write in this group"));
            label_->show();
            break;

        case DisabledPanelState::Pending:
            label_->setText(QT_TRANSLATE_NOOP("input_widget", "The join request has been sent to administrator"));
            label_->show();
            break;

        case DisabledPanelState::Deleted:
            label_->setText(QT_TRANSLATE_NOOP("input_widget", "The user is blocked"));
            label_->show();
            break;

        case DisabledPanelState::Empty:
            label_->hide();
            break;

        default:
            im_assert(!"unknown state");
            break;
        }
    }

    void InputPanelDisabled::updateStyleImpl(const InputStyleMode)
    {
        updateLabelColor();
    }

    void InputPanelDisabled::updateLabelColor()
    {
        const auto var = styleMode_ == InputStyleMode::Default
            ? Styling::StyleVariable::BASE_PRIMARY
            : Styling::StyleVariable::GHOST_PRIMARY_INVERSE;
        label_->setColor(Styling::ThemeColorKey{ var });
    }
}
