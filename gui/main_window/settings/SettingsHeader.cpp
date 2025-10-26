#include "stdafx.h"

#include "SettingsHeader.h"
#include "../../controls/TextUnit.h"
#include "../../controls/CustomButton.h"

#include "../contact_list/TopPanel.h"
#include "../../utils/utils.h"
#include "../../styles/ThemeParameters.h"
#include "../../fonts.h"

namespace Ui
{
    SettingsHeader::SettingsHeader(QWidget* _parent)
        : QWidget(_parent)
        , textUnit_(nullptr)
        , backButton_(new CustomButton(this, qsl(":/controls/back_icon")))
    {
        Styling::Buttons::setButtonDefaultColors(backButton_);
        backButton_->setFixedSize(Utils::scale_value(QSize(20, 20)));
        Testing::setAccessibleName(backButton_, qsl("AS SettingsTab backButton"));

        QObject::connect(backButton_, &CustomButton::clicked, this, [this]() { Q_EMIT backClicked(QPrivateSignal()); });

        auto layout = Utils::emptyHLayout(this);
        layout->setSpacing(0);

        layout->addSpacerItem(new QSpacerItem(Utils::scale_value(14), 0, QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Fixed));
        layout->addWidget(backButton_);
        layout->addStretch();
    }

    SettingsHeader::~SettingsHeader()
    {
    }

    void SettingsHeader::setText(const QString& _text)
    {
        const auto textColor = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };

        if (!textUnit_)
        {
            textUnit_ = TextRendering::MakeTextUnit(_text);
            TextRendering::TextUnit::InitializeParameters params{ Fonts::appFontScaled(16, Fonts::FontWeight::SemiBold), textColor };
            params.align_ = TextRendering::HorAligment::CENTER;
            params.maxLinesCount_ = 1;
            textUnit_->init(params);
        }
        else
        {
            textUnit_->setColor(textColor);
            textUnit_->setText(_text);
        }

        textUnit_->evaluateDesiredSize();
        update();
    }

    bool SettingsHeader::hasText() const
    {
        return textUnit_ && !textUnit_->getText().isEmpty();
    }

    void SettingsHeader::paintEvent(QPaintEvent*)
    {
        QPainter p(this);

        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        const auto border = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);
        Utils::drawBackgroundWithBorders(p, rect(), bg, border, Qt::AlignBottom);

        if (textUnit_)
        {
            textUnit_->setOffsets((width() - textUnit_->desiredWidth()) / 2, height() / 2);
            textUnit_->draw(p, TextRendering::VerPosition::MIDDLE);
        }
    }
}
