#include "stdafx.h"

#include "HistogramPtt.h"
#include "PlayerHistogram.h"
#include "RecorderHistogram.h"
#include "PlayButton.h"
#include "controls/TextUnit.h"
#include "controls/CustomButton.h"
#include "controls/TooltipWidget.h"
#include "fonts.h"

#ifndef STRIP_AV_MEDIA
#include "media/ptt/AudioUtils.h"
#endif // !STRIP_AV_MEDIA

#include "../InputWidgetUtils.h"
#include "../CircleHover.h"
#include "styles/ThemeParameters.h"
#include "utils/utils.h"

namespace Ui
{
    HistogramContainer::HistogramContainer(QWidget* _parent)
        : QWidget(_parent)
    {
        auto rootLayot = Utils::emptyHLayout(this);
        rootLayot->setSpacing(0);
        playerHistogram_ = new PlayerHistogram(this);
        playerHistogram_->hide();
        rootLayot->addWidget(playerHistogram_);
        recorderHistogram_ = new RecorderHistogram(this);
        rootLayot->addWidget(recorderHistogram_);
        rootLayot->setContentsMargins(0, getVerMargin(), 0, getVerMargin());
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    HistogramPtt::HistogramPtt(QWidget* _parent, std::function<std::chrono::milliseconds(int, double)> _durationCalc)
        : ClickableWidget(_parent)
        , durationCalc_(std::move(_durationCalc))
    {
        im_assert(durationCalc_);
        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->setSpacing(0);

        playButton_ = new PlayButton(this);

        auto circleHover = std::make_unique<CircleHover>();
        circleHover->setColor(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY, 0.28));
        playButton_->setCircleHover(std::move(circleHover));
        setStoppedButton();

        const auto buttonHorMargin = (getHorSpacer() * 2 + Utils::scale_value(20) - playButton_->width()) / 2;
        {
            const auto topMargin = (getDefaultInputHeight() - 2 * getVerMargin() - playButton_->height()) / 2;

            auto playButtonContainer = new QWidget(this);
            playButtonContainer->setFixedWidth(playButton_->width());
            auto layout = Utils::emptyHLayout(playButtonContainer);
            layout->setAlignment(Qt::AlignTop);
            layout->setContentsMargins(0, topMargin, 0, 0);
            layout->addWidget(playButton_);
            rootLayout->addWidget(playButtonContainer);
        }

        rootLayout->addSpacing(buttonHorMargin);

        histograms_ = new HistogramContainer(this);
        rootLayout->addWidget(histograms_);

        durationText_ = TextRendering::MakeTextUnit(Utils::getFormattedTime(std::chrono::seconds::zero()));
        durationText_->init({ Fonts::appFontScaled(16, Fonts::FontWeight::Normal), Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE } });
        durationText_->evaluateDesiredSize();
        rootLayout->setContentsMargins(buttonHorMargin, 0, durationText_->desiredWidth() + Utils::scale_value(16) + getHorSpacer(), 0);

        tooltipTimer_.setSingleShot(true);
        tooltipTimer_.setInterval(Tooltip::getDefaultShowDelay());

        connect(&tooltipTimer_, &QTimer::timeout, this, [this]()
        {
            if (isHovered() || underMouse_)
                showToolTip();
        });
        connect(playButton_, &CustomButton::clicked, this, &HistogramPtt::onPlayButtonClicked);

        connect(this, &ClickableWidget::hoverChanged, this, &HistogramPtt::updateHover);
        connect(this, &ClickableWidget::pressChanged, this, &HistogramPtt::updatePressed);
        connect(this, &ClickableWidget::clicked, this, &HistogramPtt::onClicked);
    }

    HistogramPtt::~HistogramPtt() = default;

    void HistogramPtt::start()
    {
#ifndef STRIP_AV_MEDIA
        const auto volume = ptt::getVolumeDefaultDevice();
#else
        constexpr auto volume = 1.0;
#endif // !STRIP_AV_MEDIA

        histograms_->recorderHistogram()->setVolume(volume);
        histograms_->playerHistogram()->setVolume(volume);
        histograms_->playerHistogram()->hide();
        histograms_->recorderHistogram()->show();
        histograms_->recorderHistogram()->start();
        histograms_->playerHistogram()->start();
        setStoppedButton();
    }

    void HistogramPtt::stop()
    {
        histograms_->playerHistogram()->stop();
        histograms_->recorderHistogram()->stop();
        histograms_->recorderHistogram()->hide();
        setCurrentSample(std::numeric_limits<int>::max());
        histograms_->playerHistogram()->show();
        setPlayingButton();
        update();
    }

    void HistogramPtt::switchToInit()
    {
        setDuration(std::chrono::milliseconds::zero());
        histograms_->playerHistogram()->clear();
        histograms_->playerHistogram()->hide();
        histograms_->recorderHistogram()->clear();
        histograms_->recorderHistogram()->show();
        setStoppedButton();
        update();
    }

    void HistogramPtt::setDuration(std::chrono::milliseconds _duration)
    {
        if (duration_ != _duration)
        {
            duration_ = _duration;
            durationText_->setText(Utils::getFormattedTime(_duration));
            durationText_->evaluateDesiredSize();
            update();
        }
    }

    void HistogramPtt::enableCircleHover(bool _val)
    {
        playButton_->enableCircleHover(_val);
        if (!_val)
            playButton_->updateIcon();
    }

    void HistogramPtt::setUnderLongPress(bool _val)
    {
        playButton_->setUnderLongPress(_val);
    }

    bool HistogramPtt::hasPlayButton() const
    {
        return playButton_->getState() == PlayButton::State::Play;
    }

    bool HistogramPtt::hasPauseButton() const
    {
        return playButton_->getState() == PlayButton::State::Pause;
    }

    void HistogramPtt::setPlayingButton()
    {
        playButton_->setState(PlayButton::State::Play);
    }

    void HistogramPtt::setPausedButton()
    {
        playButton_->setState(PlayButton::State::Pause);
    }

    void HistogramPtt::setStoppedButton()
    {
        playButton_->setState(PlayButton::State::Stop);
    }

    void HistogramPtt::setCurrentSample(int _current)
    {
        histograms_->playerHistogram()->setCurrentSample(_current);
    }

    void HistogramPtt::updateHover(bool _hovered)
    {
        const auto isLeftPressed = qApp->mouseButtons() == Qt::MouseButtons(Qt::LeftButton);
        if (const auto newUnderMouse = _hovered || (isLeftPressed && rect().contains(mapFromGlobal(QCursor::pos()))); newUnderMouse != underMouse_)
        {
            underMouse_ = newUnderMouse;
            playButton_->updateHover(underMouse_);
            if (underMouse_)
            {
                enableTooltip_ = true;
                tooltipTimer_.start();
            }
            else
            {
                enableTooltip_ = false;
                tooltipTimer_.stop();
                hideToolTip();
            }

            Q_EMIT underMouseChanged(underMouse_, QPrivateSignal());
            update();
        }
    }

    void HistogramPtt::onSpectrum(const QVector<double>& _v)
    {
        histograms_->recorderHistogram()->addSpectrum(_v);
        histograms_->playerHistogram()->addSpectrum(_v);
    }

    QWidget* HistogramPtt::getButtonWidget() const
    {
        return playButton_;
    }

    void HistogramPtt::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        const auto r = rect();
        const auto active = histograms_->recorderHistogram()->isActive();
        const auto hasSignal = histograms_->recorderHistogram()->hasSamples() || histograms_->playerHistogram()->hasSamples();

        auto getColor = [](auto active, auto hasSignal, auto pressed, auto underMouse)
        {
            if (active)
            {
                auto color = pressed ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_ACTIVE)
                    : (underMouse ? Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_HOVER) : Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
                return color;
            }
            else if (hasSignal)
            {
                if (pressed)
                    return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_ACTIVE);
                else if (underMouse)
                    return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY_HOVER);
                return Styling::getParameters().getColor(Styling::StyleVariable::CHAT_PRIMARY);
            }
            return Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY);
        };

        auto getTextColor = [](auto _active, auto _hasSignal)
        {
            if (_active)
                return Styling::StyleVariable::BASE_GLOBALWHITE;
            if (_hasSignal)
                return Styling::StyleVariable::TEXT_PRIMARY;
            return Styling::StyleVariable::BASE_GLOBALWHITE;
        };

        const auto color = getColor(active, hasSignal, pressed_, underMouse_);
        const auto textColor = getTextColor(active, hasSignal);

        drawInputBubble(p, r, color, 0, Utils::getShadowMargin());

        static const auto textWidth = durationText_->desiredWidth(); // calc once
        durationText_->setColor(Styling::ThemeColorKey{ textColor });
        durationText_->setOffsets(r.width() - textWidth - Utils::scale_value(16), r.height() / 2);
        durationText_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    void HistogramPtt::leaveEvent(QEvent* _e)
    {
        tooltipTimer_.stop();
        ClickableWidget::leaveEvent(_e);
    }

    void HistogramPtt::enterEvent(QEvent* _e)
    {
        enableTooltip_ = true;
        tooltipTimer_.start();
        ClickableWidget::enterEvent(_e);
    }

    void HistogramPtt::mouseMoveEvent(QMouseEvent * _e)
    {
        ClickableWidget::enterEvent(_e);
        if (histograms_->playerHistogram()->isVisible())
            showToolTip();
    }

    void HistogramPtt::updatePressed()
    {
        pressed_ = isPressed();
        if (pressed_ && !(histograms_->playerHistogram()->isVisible()))
        {
            enableTooltip_ = false;
            hideToolTip();
        }
        update();
    }

    void HistogramPtt::onClicked()
    {
        if (const auto sample = histograms_->playerHistogram()->sampleUnderCursor(); sample)
            Q_EMIT clickOnSample((*sample).idx, (*sample).coeff, QPrivateSignal());
    }

    void HistogramPtt::showToolTip()
    {
        if (enableTooltip_)
        {
            if (histograms_->recorderHistogram()->isActive())
            {
                playButton_->showToolTipForce();
            }
            else if (histograms_->playerHistogram()->isVisible())
            {
                if (const auto sample = histograms_->playerHistogram()->sampleUnderCursor(); sample)
                {
                    if (const auto durationStr = Utils::getFormattedTime(durationCalc_((*sample).idx, (*sample).coeff)); Tooltip::text() != durationStr)
                    {
                        const auto pos = mapFromGlobal(QCursor::pos());
                        auto r = rect();
                        r.setLeft(pos.x());
                        r.setRight(pos.x() + histograms_->playerHistogram()->getSampleWidth());
                        Tooltip::show(durationStr, QRect(mapToGlobal(r.topLeft()), r.size()), {0, 0}, Tooltip::ArrowDirection::Down);
                    }
                }
                else
                {
                    hideToolTip();
                }
            }
        }
    }

    void HistogramPtt::hideToolTip()
    {
        Tooltip::hide();
    }

    void HistogramPtt::onPlayButtonClicked()
    {
        switch (playButton_->getState())
        {
        case PlayButton::State::Pause:
            Q_EMIT pauseButtonClicked(QPrivateSignal());
            break;
        case PlayButton::State::Play:
            Q_EMIT playButtonClicked(QPrivateSignal());
            break;
        case PlayButton::State::Stop:
            Q_EMIT stopButtonClicked(QPrivateSignal());
            break;
        default:
            Q_UNREACHABLE();
            break;
        }
    }
}
