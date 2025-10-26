#include "ReleaseNotes.h"
#include "ReleaseNotesText.h"
#include "stdafx.h"
#include "../controls/GeneralDialog.h"

#include "../controls/TextWidget.h"
#include "history_control/MessageStyle.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../fonts.h"

#include "../controls/TransparentScrollBar.h"
#include "main_window/MainWindow.h"
#include "../styles/ThemeParameters.h"

namespace {

    auto titleTopMargin()
    {
        return Utils::scale_value(20);
    }

    auto textTopMargin()
    {
        return Utils::scale_value(28);
    }

    auto leftMargin()
    {
        return Utils::scale_value(28);
    }

    auto rightMargin()
    {
        return Utils::scale_value(60);
    }

    auto releaseNotesWidth()
    {
        return Utils::scale_value(400);
    }

    auto releaseNotesHeight()
    {
        return Utils::scale_value(414);
    }
}

namespace Ui
{

namespace ReleaseNotes
{

ReleaseNotesWidget::ReleaseNotesWidget(QWidget *_parent)
    : QWidget(_parent)
{
    setFixedSize(releaseNotesWidth(), releaseNotesHeight());

    auto layout = Utils::emptyVLayout(this);

    layout->addSpacing(titleTopMargin());

    auto titleLayout = Utils::emptyHLayout();
    auto title = new TextWidget(this, QT_TRANSLATE_NOOP("release_notes", "What's new"));
    title->init({ Fonts::appFontScaled(22, Fonts::FontWeight::SemiBold), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID } });
    titleLayout->addSpacing(leftMargin());
    Testing::setAccessibleName(title, qsl("AS ReleaseNots title"));
    titleLayout->addWidget(title);
    titleLayout->addStretch();
    layout->addLayout(titleLayout);

    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setFocusPolicy(Qt::NoFocus);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("background-color: transparent; border: none"));

    auto text = new TextWidget(this, releaseNotesText(), Data::MentionMap(), Ui::TextRendering::LinksVisible::DONT_SHOW_LINKS);
    text->setMaxWidth(releaseNotesWidth() - leftMargin() - rightMargin());
    text->init({ Fonts::appFontScaled(15, Fonts::FontWeight::Normal), MessageStyle::getTextColorKey() });

    auto scrollAreaContent = new QWidget(scrollArea);
    auto scrollAreaContentLayout = Utils::emptyVLayout(scrollAreaContent);
    auto textLayout = Utils::emptyHLayout();
    Testing::setAccessibleName(text, qsl("AS ReleaseNots text"));
    textLayout->addWidget(text);
    scrollAreaContentLayout->addLayout(textLayout);
    scrollAreaContentLayout->addStretch();
    scrollAreaContent->setStyleSheet(ql1s("background-color: transparent"));

    scrollArea->setWidget(scrollAreaContent);

    layout->addSpacing(textTopMargin());
    Testing::setAccessibleName(scrollArea, qsl("AS ReleaseNots scrollArea"));
    layout->addWidget(scrollArea);
}

void showReleaseNotes()
{
    GeneralDialog::Options opt;
    opt.fixedSize_ = false;
    GeneralDialog d(new ReleaseNotesWidget(), Utils::InterConnector::instance().getMainWindow(), opt);
    d.addAcceptButton(QT_TRANSLATE_NOOP("release_notes", "OK"), true);
    d.execute();
}

QString releaseNotesHash()
{
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(releaseNotesText().toUtf8());
    return QString::fromUtf8(hash.result().toHex());
}

}

}
