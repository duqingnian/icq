#include "EditDescriptionWidget.h"
#include "SidebarUtils.h"

#include "../../controls/TextUnit.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/TextBrowserEx.h"
#include "../../controls/DialogButton.h"
#include "../../core_dispatcher.h"
#include "../../utils/utils.h"
#include "../../utils/Text.h"
#include "../../utils/InterConnector.h"
#include "../../gui_settings.h"
#include "../../styles/ThemeParameters.h"
#include "../../main_window/FilesWidget.h"

namespace
{
    auto getWidgetWidth() noexcept
    {
        return Utils::scale_value(340);
    }

    auto getWidgetHeight() noexcept
    {
        return Utils::scale_value(190);
    }

    auto getHeaderVerOffset() noexcept
    {
        return Utils::scale_value(11);
    }

    auto getHeaderHorOffset() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getLeftMargin() noexcept
    {
        return Utils::scale_value(20);
    }

    auto getTopMargin() noexcept
    {
        return Utils::scale_value(68);
    }

    auto getRightMargin() noexcept
    {
        return Utils::scale_value(28);
    }

    auto getBottomSpacing() noexcept
    {
        return Utils::scale_value(30);
    }

    auto getTextToLineSpacing() noexcept
    {
        return Utils::scale_value(7);
    }

    auto getHintHorOffset() noexcept
    {
        return Utils::scale_value(21);
    }

    auto getHintVerOffset() noexcept
    {
        return Utils::scale_value(9);
    }

    auto getCounterVerOffset() noexcept
    {
        return Utils::scale_value(9);
    }

    auto getHeaderFont()
    {
        return Fonts::appFontScaled(23);
    }

    auto getDescriptionFont()
    {
        return Fonts::appFontScaled(16);
    }

    auto getHintFont()
    {
        return Fonts::appFontScaled(15);
    }

    constexpr auto getMaxDescriptionLength() noexcept
    {
        return 120;
    }

    auto getMaxDescriptionHeight() noexcept
    {
        return Utils::scale_value(80);
    }

    auto getHintColorNormal()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_PRIMARY };
    }

    auto getHintColorBad()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::SECONDARY_ATTENTION };
    }

    auto getDescriptionColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }
}

namespace Ui
{
    EditDescriptionWidget::EditDescriptionWidget(QWidget *_parent, const FormData& _initData)
        : QWidget(_parent)
        , okButton_(nullptr)
        , cancelButton_(nullptr)
        , descriptionHeight_(-1)
        , modifiedViewPort_(false)
        , wrongDescription_(false)
    {
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed);
        setFixedWidth(getWidgetWidth());
        setFixedHeight(getWidgetHeight());

        auto globalLayout = Utils::emptyVLayout(this);
        globalLayout->setContentsMargins(getLeftMargin(), getTopMargin(), getRightMargin(), 0);

        headerUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Edit description"));
        headerUnit_->init({ getHeaderFont(), Styling::ThemeColorKey{Styling::StyleVariable::TEXT_SOLID} });
        headerUnit_->evaluateDesiredSize();
        headerUnit_->setOffsets(getHeaderHorOffset(), getHeaderVerOffset());

        const auto descriptionWidth = getWidgetWidth() - getLeftMargin() - getRightMargin();
        description_ = new TextEditEx(this, getDescriptionFont(), getDescriptionColorKey(), true, false);
        QSizePolicy sp(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        sp.setVerticalStretch(1);
        description_->setSizePolicy(sp);
        description_->setFixedWidth(descriptionWidth);
        description_->setDocumentMargin(0);
        description_->document()->setTextWidth(descriptionWidth);
        description_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        description_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        description_->setTextInteractionFlags(Qt::TextEditable | Qt::TextEditorInteraction);
        description_->setFrameStyle(QFrame::NoFrame);
        description_->setPlaceholderText(QT_TRANSLATE_NOOP("profile_edit_dialogs", "About me"));
        description_->setAcceptDrops(false);
        description_->setPlainText(_initData.description_, false);
        description_->setEnterKeyPolicy(TextEditEx::EnterKeyPolicy::FollowSettingsRules);
        Utils::ApplyStyle(description_, Styling::getParameters().getTextEditCommonQss(true));
        Testing::setAccessibleName(description_, qsl("AS ProfilePage aboutMeEdit"));

        hintUnit_ = TextRendering::MakeTextUnit(QT_TRANSLATE_NOOP("profile_edit_dialogs", "A short text about you"));
        hintUnit_->init({ getHintFont(), getHintColorNormal() });
        hintUnit_->evaluateDesiredSize();

        counterUnit_ = TextRendering::MakeTextUnit(QString::number(getMaxDescriptionLength() - _initData.description_.length()));
        counterUnit_->init({ getDescriptionFont(), getHintColorNormal() });
        counterUnit_->evaluateDesiredSize();

        connect(description_, &TextEditEx::textChanged, this, &EditDescriptionWidget::onFormChanged);
        connect(description_, &TextEditEx::enter, this, &EditDescriptionWidget::okClicked);

        globalLayout->addWidget(description_);
        globalLayout->addStretch();
        globalLayout->addSpacing(getBottomSpacing());

        onFormChanged();

        description_->setFocus();
    }

    EditDescriptionWidget::~EditDescriptionWidget()
    {
    }

    EditDescriptionWidget::FormData EditDescriptionWidget::getFormData() const
    {
        return { description_->getPlainText().trimmed() };
    }

    void EditDescriptionWidget::clearData()
    {
        description_->clear();
    }

    void EditDescriptionWidget::setButtonsPair(const QPair<DialogButton*, DialogButton*> &_buttonsPair)
    {
        im_assert(!okButton_ && !cancelButton_);

        okButton_ = _buttonsPair.first;
        cancelButton_ = _buttonsPair.second;

        connect(okButton_, &DialogButton::clicked, this, &EditDescriptionWidget::okClicked);
        connect(cancelButton_, &DialogButton::clicked, this, &EditDescriptionWidget::cancelClicked);

        okButton_->setEnabled(isFormComplete());
    }

    void EditDescriptionWidget::onFormChanged()
    {
        if (description_->getPlainText().length() > getMaxDescriptionLength())
        {
            if (!wrongDescription_)
            {
                hintUnit_->setText(QT_TRANSLATE_NOOP("profile_edit_dialogs", "Maximum length - %1 symbols").arg(getMaxDescriptionLength()), getHintColorBad());
                Utils::ApplyStyle(description_, Styling::getParameters().getTextEditBadQss());
                wrongDescription_ = true;
            }

            if (okButton_)
                okButton_->setEnabled(false);
        }
        else
        {
            if (wrongDescription_)
            {
                hintUnit_->setText(QT_TRANSLATE_NOOP("profile_edit_dialogs", "A short text about you"), getHintColorNormal());
                Utils::ApplyStyle(description_, Styling::getParameters().getTextEditCommonQss(true));
                wrongDescription_ = false;
            }

            if (okButton_)
                okButton_->setEnabled(isFormComplete());
        }

        updateDescriptionHeight();
        counterUnit_->setText(QString::number(getMaxDescriptionLength() - description_->getPlainText().length()), wrongDescription_ ? getHintColorBad() : getHintColorNormal());
        hintUnit_->elide(getWidgetWidth() - getHintHorOffset()- getRightMargin() - counterUnit_->cachedSize().width());

        update();
    }

    void EditDescriptionWidget::okClicked()
    {
        if (wrongDescription_)
            return;

        Utils::UpdateProfile({std::make_pair(std::string("aboutMe"), description_->getPlainText().trimmed())});
        Q_EMIT changed();
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditDescriptionWidget::cancelClicked()
    {
        Q_EMIT Utils::InterConnector::instance().closeAnyPopupWindow(Utils::CloseWindowInfo());
        close();
    }

    void EditDescriptionWidget::paintEvent(QPaintEvent *_event)
    {
        QWidget::paintEvent(_event);

        QPainter p(this);
        headerUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);

        hintUnit_->setOffsets(getHintHorOffset(), getTopMargin() + description_->rect().bottom() + getHintVerOffset());
        hintUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);

        counterUnit_->setOffsets(getWidgetWidth() - getRightMargin() - counterUnit_->cachedSize().width(), getTopMargin() + description_->rect().bottom() + getCounterVerOffset());
        counterUnit_->draw(p, Ui::TextRendering::VerPosition::TOP);
    }

    void EditDescriptionWidget::showEvent(QShowEvent* _e)
    {
        QWidget::showEvent(_e);
        description_->setFocus();
    }

    bool EditDescriptionWidget::isFormComplete() const
    {
        return true;
    }

    void EditDescriptionWidget::updateDescriptionHeight()
    {
        const auto newHeight = description_->getTextHeight();

        if ((descriptionHeight_ != -1 && newHeight == descriptionHeight_) || newHeight == 0)
            return;

        static const auto minHeight = QFontMetrics(getDescriptionFont()).height();
        static const auto maxHeight = getMaxDescriptionHeight();

        if (newHeight > maxHeight)
        {
            if (!modifiedViewPort_)
            {
                description_->setViewportMargins(0, 0, 0, getTextToLineSpacing());
                modifiedViewPort_ = true;
            }
        }
        else
        {
            if (modifiedViewPort_)
            {
                description_->setViewportMargins(0, 0, 0, 0);
                modifiedViewPort_ = false;
            }
        }

        descriptionHeight_ = std::clamp(newHeight, minHeight, maxHeight);

        description_->setVerticalScrollBarPolicy(modifiedViewPort_ ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        description_->setFixedHeight(descriptionHeight_ + getTextToLineSpacing());

        if (modifiedViewPort_)
            descriptionHeight_ = newHeight;

        update();
    }
}
