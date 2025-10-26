#include "stdafx.h"

#include "CheckForUpdateButton.h"
#include "controls/ClickWidget.h"
#include "controls/CustomButton.h"
#include "previewer/toast.h"
#include "../ConnectionWidget.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../fonts.h"
#include "styles/ThemeParameters.h"
#include "../../core_dispatcher.h"
#include "../MainWindow.h"

namespace
{
    auto getIconsSize()
    {
        return QSize(32, 32);
    }

    auto getIconsColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY };
    }

    auto getSpinnerPenWidth()
    {
        return Utils::fscale_value(1.3);
    }

    auto getSpinnerWidth() noexcept
    {
        return Utils::fscale_value(18.0);
    }

    constexpr auto defaultSeq = -1;
}

namespace Ui
{
    CheckForUpdateButton::CheckForUpdateButton(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(Utils::scale_value(getIconsSize()).height());
        auto layout = Utils::emptyHLayout(this);
        layout->setSpacing(0);
        layout->setContentsMargins({});

        icon_ = new CustomButton(this);
        icon_->setDefaultImage(qsl(":/controls/reload"), getIconsColorKey(), getIconsSize());
        icon_->setFixedSize(Utils::scale_value(getIconsSize()));
        icon_->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        icon_->setCursor(Qt::ArrowCursor);

        layout->addWidget(icon_);

        animation_ = new ProgressAnimation(this);
        animation_->setProgressWidth(getSpinnerWidth());
        animation_->setFixedSize(getIconsSize());
        animation_->hide();
        layout->addWidget(animation_, Qt::AlignVCenter);

        layout->addSpacing(Utils::scale_value(4));

        const auto str = QT_TRANSLATE_NOOP("about_us", "Check for updates");
        text_ = new ClickableTextWidget(this, Fonts::appFontScaled(15), Styling::ThemeColorKey{ Styling::StyleVariable::PRIMARY });
        text_->setText(str);
        text_ ->setFixedHeight(Utils::scale_value(getIconsSize()).height());
        QObject::connect(text_, &ClickableTextWidget::clicked, this, &CheckForUpdateButton::onClick);
        layout->addWidget(text_);
        layout->addSpacerItem(new QSpacerItem(1, 1, QSizePolicy::Policy::Expanding));

        QObject::connect(GetDispatcher(), &core_dispatcher::updateReady, this, &CheckForUpdateButton::onUpdateReady);
        QObject::connect(GetDispatcher(), &core_dispatcher::upToDate, this, &CheckForUpdateButton::onUpToDate);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::onMacUpdateInfo, this, [this](Utils::MacUpdateState _state)
        {
            if (_state == Utils::MacUpdateState::Requested)
                stopAnimation();
            else if (_state == Utils::MacUpdateState::Ready)
                onUpdateReady();
            else
                onUpToDate(defaultSeq, _state == Utils::MacUpdateState::LoadError);
        });
        Testing::setAccessibleName(text_, qsl("AS AboutUsPage checkForUpdatesButton"));
    }

    CheckForUpdateButton::~CheckForUpdateButton() = default;

    void CheckForUpdateButton::onClick()
    {
        if (isUpdateReady_)
        {
            showUpdateReadyToast();
        }
        else if (!seq_)
        {
            startAnimation();
#ifdef __APPLE__
            seq_ = defaultSeq;
            Utils::InterConnector::instance().getMainWindow()->checkForUpdatesInBackground();
#else
            seq_ = GetDispatcher()->post_message_to_core("update/check", nullptr);
#endif
        }
    }

    void CheckForUpdateButton::onUpdateReady()
    {
        isUpdateReady_ = true;
        seq_ = std::nullopt;
        stopAnimation();
        showUpdateReadyToast();
    }

    void CheckForUpdateButton::onUpToDate(int64_t _seq, bool _isNetworkError)
    {
        if (seq_ == _seq)
        {
            seq_ = std::nullopt;
            stopAnimation();
            const auto text = _isNetworkError ? QT_TRANSLATE_NOOP("about_us", "Server error") : QT_TRANSLATE_NOOP("about_us", "You have the latest version");
            Utils::showTextToastOverContactDialog(text);
        }
    }

    void CheckForUpdateButton::startAnimation()
    {
        icon_->hide();
        animation_->setProgressPenColorKey(getIconsColorKey());
        animation_->setProgressPenWidth(getSpinnerPenWidth());
        animation_->start();
        animation_->show();
    }

    void CheckForUpdateButton::stopAnimation()
    {
        icon_->show();
        animation_->stop();
        animation_->hide();
    }

    void CheckForUpdateButton::showUpdateReadyToast()
    {
        Utils::showTextToastOverContactDialog(QT_TRANSLATE_NOOP("about_us", "Update required"));
    }
}
