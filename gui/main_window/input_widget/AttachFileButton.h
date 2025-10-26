#pragma once

#include "controls/ClickWidget.h"
#include "InputWidgetUtils.h"

namespace Ui
{
    class AttachFileButton : public ClickableWidget, public StyledInputElement
    {
        Q_OBJECT

    public:
        AttachFileButton(QWidget* _parent);

        void setActive(const bool _isActive);
        bool isActive() const noexcept { return isActive_; }

        void onAttachVisibleChanged(bool _isVisible);

    protected:
        void paintEvent(QPaintEvent*) override;
        void updateStyleImpl(const InputStyleMode _mode) override;

    private:
        enum class RotateDirection
        {
            Left,
            Right,
        };
        void rotate(const RotateDirection _dir);

    private:
        QVariantAnimation* anim_;
        double currentAngle_;
        bool isActive_;
    };
}