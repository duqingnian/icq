#include "NickLineEdit.h"

#include "TextEditEx.h"
#include "TextUnit.h"
#include "../fonts.h"
#include "../main_window/ConnectionWidget.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../core_dispatcher.h"
#include "../styles/ThemeParameters.h"

#include "../omicron/omicron_helper.h"
#include "../common.shared/omicron_keys.h"

namespace
{
    auto getTextToLineSpacing() noexcept
    {
        return Utils::scale_value(7);
    }

    auto getCommonTopSpacingUnscaled() noexcept
    {
        if constexpr (platform::is_apple())
            return 16;
        else
            return 9;
    }

    auto getSpinnerSpacing() noexcept
    {
        if constexpr (platform::is_apple())
            return Utils::scale_value(getCommonTopSpacingUnscaled() - 5);
        else
            return Utils::scale_value(getCommonTopSpacingUnscaled() + 1);
    }

    auto getHintSpacing() noexcept
    {
        return Utils::scale_value(getCommonTopSpacingUnscaled());
    }

    auto getCounterSpacing() noexcept
    {
        return Utils::scale_value(getCommonTopSpacingUnscaled() + 1);
    }

    auto getProgressAnimationSize() noexcept
    {
        return Utils::fscale_value(13.8);
    }

    auto getProgressAnimationPenSize() noexcept
    {
        return Utils::fscale_value(1.3);
    }

    auto getProgressHintHorOffset() noexcept
    {
        return Utils::scale_value(21);
    }

    auto getNickFont()
    {
        return Fonts::appFontScaled(16);
    }

    auto getHintFont()
    {
        return Fonts::appFontScaled(15);
    }

    auto getMinNickLength()
    {
        return Omicron::_o(omicron::keys::profile_nickname_minimum_length, feature::default_profile_nickname_minimum_length());
    }

    auto getMaxNickLength()
    {
        return Omicron::_o(omicron::keys::profile_nickname_maximum_length, feature::default_profile_nickname_maximum_length());
    }

    constexpr std::chrono::milliseconds getCheckTimeout() noexcept
    {
        return std::chrono::seconds(1);
    }

    constexpr auto getServerResendCount() noexcept
    {
        return 2;
    }

    auto getProgressAnimationPenColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto getHintColorNormal()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto getHintColorBad()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION };
    }

    auto getHintColorAvailable()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_RAINBOW_MINT };
    }
}

namespace Ui
{
    NickLineEdit::NickLineEdit(QWidget* _parent, const QString& _initNick, const QString& _fixedPart, bool _groupMode)
        : QWidget(_parent)
        , originNick_(_initNick)
        , fixedPart_(_fixedPart)
        , previous_(makeHtml(_fixedPart, _initNick))
        , previousPlain_(_initNick % _fixedPart)
        , hintHorOffset_(0)
        , progressAnimation_(nullptr)
        , checkTimer_(new QTimer(this))
        , checkResult_(-1)
        , serverErrorsCounter_(0)
        , lastReqId_(-1)
        , lastServerRequest_(ServerRequest::None)
        , linkPressed_(false)
        , isFrendlyMinLengthError_(_initNick.isEmpty())
        , groupMode_(_groupMode)
    {
        setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::MinimumExpanding);

        auto globalLayout = Utils::emptyVLayout(this);
        globalLayout->setContentsMargins(QMargins());

        nick_ = new TextEditEx(this, getNickFont(), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID }, true, false);
        nick_->setPlaceholderText(QT_TRANSLATE_NOOP("nick_edit", "Come up with a unique nickname"));
        nick_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Fixed);
        nick_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        nick_->setAttribute(Qt::WA_MacShowFocusRect, false);
        nick_->setContextMenuPolicy(Qt::ContextMenuPolicy::NoContextMenu);
        nick_->setTextInteractionFlags(Qt::TextEditorInteraction);
        nick_->document()->setMaximumBlockCount(1);
        nick_->setWordWrapMode(QTextOption::NoWrap);
        nick_->setText(previous_);
        nick_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        nick_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        nick_->verticalScrollBar()->setEnabled(false);
        nick_->setContentsMargins(0, 0, 0, 0);
        Utils::ApplyStyle(nick_, Styling::getParameters().getTextLineEditCommonQss(false, Utils::unscale_value(QFontMetrics(getNickFont()).height() + getTextToLineSpacing())));
        Testing::setAccessibleName(nick_, qsl("AS General nickEdit"));

        auto hintLayout = Utils::emptyHLayout();
        hintLayout->setContentsMargins(QMargins());

        progressAnimation_ = new ProgressAnimation(this);
        progressAnimation_->setProgressWidth(getProgressAnimationSize());
        progressAnimation_->setProgressPenWidth(getProgressAnimationPenSize());
        progressAnimation_->setProgressPenColorKey(getProgressAnimationPenColorKey());
        progressAnimation_->setFixedWidth(getProgressAnimationSize() + Utils::scale_value(2));
        hintLayout->addWidget(progressAnimation_);
        hintLayout->addStretch();

        counterUnit_ = TextRendering::MakeTextUnit(QString());
        TextRendering::TextUnit::InitializeParameters params{ getHintFont(), getHintColorNormal() };
        counterUnit_->init(params);
        updateCounter();

        hintUnit_ = TextRendering::MakeTextUnit(QString());
        params.linkColor_ = getHintColorBad();
        hintUnit_->init(params);

        globalLayout->addWidget(nick_);
        globalLayout->addSpacing(getSpinnerSpacing());
        globalLayout->addLayout(hintLayout);

        checkTimer_->setSingleShot(true);
        checkTimer_->setInterval(getCheckTimeout());

        connect(nick_, &TextEditEx::textChanged, this, &NickLineEdit::onNickChanged);
        connect(nick_, &TextEditEx::cursorPositionChanged, this, &NickLineEdit::onNickCursorPositionChanged);
        connect(checkTimer_, &QTimer::timeout, this, &NickLineEdit::onCheckTimeout);

        connect(GetDispatcher(), &Ui::core_dispatcher::nickCheckSetResult, this, &NickLineEdit::onNickCheckResult);
        connect(GetDispatcher(), &Ui::core_dispatcher::groupNickCheckSetResult, this, &NickLineEdit::onNickCheckResult);

        setMouseTracking(true);
    }

    NickLineEdit::~NickLineEdit()
    {
    }

    QSize NickLineEdit::sizeHint() const
    {
        const auto hintHeight = hintUnit_->getHeight(nick_->width() - hintHorOffset_ - counterUnit_->cachedSize().width() - getHintSpacing());
        const auto height = nick_->height() + qMax(getHintSpacing(), getCounterSpacing()) + qMax(hintHeight, counterUnit_->cachedSize().height());

        return QSize(width(), height);
    }

    void NickLineEdit::setText(const QString& _nick, bool _silent)
    {
        if (_silent)
            nick_->blockSignals(true);

        previous_ = makeHtml(fixedPart_, _nick);
        previousPlain_ = fixedPart_ % _nick;
        nick_->setText(previous_);

        if (_silent)
            nick_->blockSignals(false);

        moveCursorToTheEnd();
    }

    QString NickLineEdit::getText(bool full) const
    {
        auto text = nick_->getPlainText();
        if (full)
            return text;

        return text.mid(fixedPart_.size(), text.size() - fixedPart_.size());
    }

    void NickLineEdit::setNickRequest()
    {
        checkResult_ = -1;
        lastReqId_ = groupMode_ ? GetDispatcher()->checkGroupNickname(getText()) : GetDispatcher()->checkNickname(getText(), true);
        lastServerRequest_ = ServerRequest::SetNick;
    }

    void NickLineEdit::cancelSetNick()
    {
        checkResult_ = -1;
        lastReqId_ = -1;
        lastServerRequest_ = ServerRequest::None;
    }

    void NickLineEdit::resetInput()
    {
        nick_->clear();
    }

    void NickLineEdit::setFocus()
    {
        nick_->setFocus();
        moveCursorToTheEnd();
    }

    void NickLineEdit::onNickChanged()
    {
        QSignalBlocker sb(nick_);
        const auto plainText = nick_->getPlainText();
        auto text = QStringView(plainText);
        if (!fixedPart_.isEmpty() && !text.startsWith(fixedPart_))
        {
            auto cur = nick_->textCursor();
            auto pos = cur.position();
            if (text.isEmpty())
            {
                previous_ = makeHtml(fixedPart_, {});
                previousPlain_ = fixedPart_;
                cur.movePosition(QTextCursor::End);
            }

            nick_->setText(previous_);

            if (!text.isEmpty())
            {
                if (text.size() > previousPlain_.size())
                    pos -= (text.size() - previousPlain_.size());
                else
                    pos += (previousPlain_.size() - text.size());

                cur.setPosition(pos);
            }

            nick_->setTextCursor(cur);
            return;
        }

        Q_EMIT changed();

        text = text.mid(fixedPart_.size(), text.size() - fixedPart_.size());
        auto cur = nick_->textCursor();
        auto pos = cur.position();
        previous_ = makeHtml(fixedPart_, text);
        previousPlain_ = fixedPart_ % text;
        nick_->setText(previous_);
        cur.setPosition(pos);
        nick_->setTextCursor(cur);

        updateNickLine(false);
        updateCounter();

        checkResult_ = -1;
        lastReqId_ = -1;

        if (text.isEmpty())
        {
            checkTimer_->stop();
            stopCheckAnimation();
            updateHint(HintTextCode::NickMinLength, getHintColorNormal());
            isFrendlyMinLengthError_ = true;
            Q_EMIT ready();
        }
        else
        {
            checkTimer_->start();

            if (!progressAnimation_->isStarted())
                startCheckAnimation();

            if (checkForCorrectInput())
                serverCheckNickname();
            else
                checkResult_ = static_cast<int>(core::nickname_errors::bad_value);
        }

        update();
    }

    void NickLineEdit::onNickCursorPositionChanged()
    {
        auto cur = nick_->textCursor();
        if (!fixedPart_.isEmpty() && cur.position() < fixedPart_.length())
        {
            cur.setPosition(fixedPart_.length(), cur.hasSelection() ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
            nick_->setTextCursor(cur);
        }

        update();
    }

    void NickLineEdit::onNickCheckResult(qint64 _reqId, int _error)
    {
        if (lastReqId_ != _reqId)
            return;

        checkResult_ = _error;
        const auto nickErrors = static_cast<core::nickname_errors>(checkResult_);

        if (nickErrors != core::nickname_errors::unknown_error)
            serverErrorsCounter_ = 0;

        if (lastServerRequest_ == ServerRequest::SetNick)
        {
            const auto color = getHintColorBad();
            switch (nickErrors)
            {
            case core::nickname_errors::success:
                Q_EMIT nickSet();
                return;
            case core::nickname_errors::bad_value:
                updateHint(HintTextCode::InvalidChar, color);
                Q_EMIT serverError();
                break;
            case core::nickname_errors::already_used:
                updateHint(HintTextCode::NickAlreadyOccupied, color);
                Q_EMIT serverError();
                break;
            case core::nickname_errors::not_allowed:
                updateHint(HintTextCode::NickNotAllowed, color);
                Q_EMIT serverError();
                break;
            default:
                if (serverErrorsCounter_++ < getServerResendCount())
                {
                    setNickRequest();
                    return;
                }
                else
                {
                    serverErrorsCounter_ = 0;
                    updateHint(HintTextCode::ServerError, color);
                    Q_EMIT serverError(true);
                }
                break;
            }
            updateNickLine(true);
        }
    }

    void NickLineEdit::onCheckTimeout()
    {
        const auto nick = getText();

        if (isFrendlyMinLengthError_ && nick.length() >= getMinNickLength())
            isFrendlyMinLengthError_ = false;

        if (nick.compare(originNick_, Qt::CaseInsensitive) == 0)
        {
            lastReqId_ = -1;
            checkResult_ = -1;
            stopCheckAnimation();
            hintUnit_->setText(QString());
            update();
            Q_EMIT ready();
            Q_EMIT sameNick();
            return;
        }

        if (checkResult_ < 0)
        {
            checkTimer_->start();
            return;
        }

        bool servError = false;
        switch (static_cast<core::nickname_errors>(checkResult_))
        {
        case core::nickname_errors::success:
            updateHint(HintTextCode::NickAvailable, getHintColorAvailable());
            Q_EMIT ready();
            break;
        case core::nickname_errors::bad_value:
            if (checkForCorrectInput(true))
                updateHint(HintTextCode::InvalidNick, getHintColorBad());
            break;
        case core::nickname_errors::already_used:
            updateHint(HintTextCode::NickAlreadyOccupied, getHintColorBad());
            break;
        case core::nickname_errors::not_allowed:
            updateHint(HintTextCode::NickNotAllowed, getHintColorBad());
            break;
        default:
            if (serverErrorsCounter_++ < getServerResendCount())
            {
                serverCheckNickname(true);
                return;
            }
            else
            {
                servError = true;
                serverErrorsCounter_ = 0;
                setServerErrorHint();
            }
            break;
        }

        stopCheckAnimation();

        updateNickLine(checkResult_ > 0);

        if (servError)
            Q_EMIT serverError();
    }

    void NickLineEdit::paintEvent(QPaintEvent* _event)
    {
        QWidget::paintEvent(_event);

        QPainter p(this);

        hintUnit_->setOffsets(hintHorOffset_, nick_->rect().bottom() + getHintSpacing());
        hintUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);

        counterUnit_->setOffsets(width() - counterUnit_->cachedSize().width(), nick_->rect().bottom() + getCounterSpacing());
        counterUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
    }

    void NickLineEdit::mouseMoveEvent(QMouseEvent* _event)
    {
        const auto overLink = hintUnit_->isOverLink(_event->pos());
        setCursor(overLink ? Qt::PointingHandCursor : Qt::ArrowCursor);

        QWidget::mouseMoveEvent(_event);
    }

    void NickLineEdit::mousePressEvent(QMouseEvent* _event)
    {
        linkPressed_ = hintUnit_->isOverLink(_event->pos());

        _event->accept();
        QWidget::mousePressEvent(_event);
    }

    void NickLineEdit::mouseReleaseEvent(QMouseEvent* _event)
    {
        const auto overLink = hintUnit_->isOverLink(_event->pos());

        if (overLink && linkPressed_)
            retryLastServerRequest();

        linkPressed_ = false;

        _event->accept();
        QWidget::mouseReleaseEvent(_event);
    }

    void NickLineEdit::resizeEvent(QResizeEvent * _event)
    {
        adjustSize();
        updateGeometry();
        QWidget::resizeEvent(_event);
    }

    void NickLineEdit::serverCheckNickname(bool _recheck)
    {
        if (_recheck)
        {
            checkResult_ = -1;
            checkTimer_->start();

            if (!progressAnimation_->isStarted())
                startCheckAnimation();
        }

        lastReqId_ = groupMode_ ? GetDispatcher()->checkGroupNickname(getText()) : GetDispatcher()->checkNickname(getText(), false);
        lastServerRequest_ = ServerRequest::CheckNick;
    }

    void NickLineEdit::updateHint(HintTextCode _code, const Styling::ThemeColorKey& _color)
    {
        QString text;
        switch (_code)
        {
        case HintTextCode::InvalidNick:
            text = QT_TRANSLATE_NOOP("nick_edit", "Invalid nickname");
            break;
        case HintTextCode::InvalidChar:
            text = QT_TRANSLATE_NOOP("nick_edit", "Invalid symbol");
            break;
        case HintTextCode::NickAlreadyOccupied:
            text = QT_TRANSLATE_NOOP("nick_edit", "This nickname is taken");
            break;
        case HintTextCode::NickNotAllowed:
            text = QT_TRANSLATE_NOOP("nick_edit", "For agent account nickname is not allowed");
            break;
        case HintTextCode::NickAvailable:
            text = QT_TRANSLATE_NOOP("nick_edit", "Nickname available");
            break;
        case HintTextCode::NickMinLength:
            text = QT_TRANSLATE_NOOP("nick_edit", "Minimum length - %1 symbols").arg(getMinNickLength());
            break;
        case HintTextCode::NickMaxLength:
            text = QT_TRANSLATE_NOOP("nick_edit", "Maximum length - %1 symbols").arg(getMaxNickLength());
            break;
        case HintTextCode::NickLatinChar:
            text = QT_TRANSLATE_NOOP("nick_edit", "Nicknames must start with a letter of the roman alphabet (a-z)");
            break;
        case HintTextCode::CheckNick:
            text = QT_TRANSLATE_NOOP("nick_edit", "Checking...");
            break;
        case HintTextCode::ServerError:
            text = QT_TRANSLATE_NOOP("nick_edit", "Server error");
            break;
        default:
            break;
        }

        hintUnit_->setText(text, _color);

        if (_color == getHintColorBad())
            counterUnit_->setColor(_color);

        updateGeometry();
        update();
    }

    void NickLineEdit::updateCounter()
    {
        counterUnit_->setText(QString::number(getMaxNickLength() - getText().length()), getHintColorNormal());
        update();
    }

    void NickLineEdit::clearHint()
    {
        hintUnit_->setText(QString());
        updateNickLine(false);
        update();
    }

    void NickLineEdit::setServerErrorHint()
    {
        const auto linkText = QT_TRANSLATE_NOOP("nick_edit", "Repeat verification");
        const std::map<QString, QString> links = { {linkText, QString()} };
        auto linkUnit = TextRendering::MakeTextUnit(linkText);
        TextRendering::TextUnit::InitializeParameters params{ getHintFont(), getHintColorBad() };
        params.linkColor_ = getHintColorBad();
        linkUnit->init(params);

        linkUnit->setUnderline(true);
        linkUnit->applyLinks(links);

        hintUnit_->setText(QT_TRANSLATE_NOOP("nick_edit", "Server error. "), getHintColorBad());
        hintUnit_->append(std::move(linkUnit));

        updateGeometry();
        update();
    }

    void NickLineEdit::updateNickLine(bool _isError)
    {
        Utils::ApplyStyle(nick_, Styling::getParameters().getTextLineEditCommonQss(_isError, Utils::unscale_value(QFontMetrics(getNickFont()).height() + getTextToLineSpacing())));
    }

    bool NickLineEdit::checkForCorrectInput(bool _showHints)
    {
        static const QRegularExpression nickRule(
            qsl("^[a-zA-Z][a-zA-Z0-9._]*$"),
            QRegularExpression::UseUnicodePropertiesOption);
        const auto nick = getText();
        const auto color = getHintColorBad();

        if (!nickRule.match(nick).hasMatch())
        {
            static const QRegularExpression startsWithLatin(
                qsl("^[a-zA-Z]"),
                QRegularExpression::UseUnicodePropertiesOption);

            if (_showHints)
            {
                if (startsWithLatin.match(nick).hasMatch())
                    updateHint(HintTextCode::InvalidChar, color);
                else
                    updateHint(HintTextCode::NickLatinChar, color);
            }

            Q_EMIT checkError();
            return false;
        }

        if (nick.length() < getMinNickLength())
        {
            if (_showHints)
            {
                if (isFrendlyMinLengthError_)
                {
                    updateHint(HintTextCode::NickMinLength, getHintColorNormal());
                    checkResult_ = 0;
                }
                else
                {
                    updateHint(HintTextCode::NickMinLength, color);
                }
            }
            Q_EMIT checkError();
            return false;
        }
        else if (nick.length() > getMaxNickLength())
        {
            if (_showHints)
                updateHint(HintTextCode::NickMaxLength, color);
            Q_EMIT checkError();
            return false;
        }

        return true;
    }

    void NickLineEdit::startCheckAnimation()
    {
        progressAnimation_->start();
        hintHorOffset_ = getProgressHintHorOffset();
        updateHint(HintTextCode::CheckNick, getHintColorNormal());
    }

    void NickLineEdit::stopCheckAnimation()
    {
        progressAnimation_->stop();
        hintHorOffset_ = 0;
    }

    void NickLineEdit::retryLastServerRequest()
    {
        serverErrorsCounter_ = 0;
        stopCheckAnimation();
        updateNickLine(false);

        switch (lastServerRequest_)
        {
        case ServerRequest::CheckNick:
            serverCheckNickname(true);
            break;
        case ServerRequest::SetNick:
            setNickRequest();
            break;
        default:
            break;
        }
    }

    void NickLineEdit::moveCursorToTheEnd()
    {
        auto cur = nick_->textCursor();
        cur.movePosition(QTextCursor::End);
        nick_->setTextCursor(cur);
    }

    QString NickLineEdit::makeHtml(QStringView _fixed, QStringView _text)
    {
        auto color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_PRIMARY).name(QColor::HexRgb);
        return u"<font color=" % color % u'>' % _fixed % u"</font>" % _text;
    }
}
