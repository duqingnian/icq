#include "stdafx.h"

#include "WallpaperPreviewWidget.h"
#include "controls/BackgroundWidget.h"
#include "utils/utils.h"
#include "utils/InterConnector.h"
#include "styles/ThemesContainer.h"

#include "main_window/history_control/complex_message/ComplexMessageItemBuilder.h"
#include "main_window/history_control/complex_message/ComplexMessageItem.h"

namespace
{
    template <typename T>
    void foreachMessage(QWidget* _parent, const T& _func)
    {
        const auto widgets = _parent->findChildren<Ui::ComplexMessage::ComplexMessageItem*>();
        for (auto msg : widgets)
            _func(msg);
    }
}

namespace Ui
{
    using namespace ComplexMessage;

    WallpaperPreviewWidget::WallpaperPreviewWidget(QWidget* _parent, const PreviewMessagesVector& _messages)
        : QWidget(_parent)
        , bg_(new BackgroundWidget(this, false))
    {
        auto rootLayout = Utils::emptyHLayout(this);
        rootLayout->addWidget(bg_);

        auto pageLayout = Utils::emptyVLayout(bg_);

        for (const auto& i: _messages)
        {
            Data::MessageBuddy msg;
            msg.SetText(i.text_);
            msg.SetDate(QDate::currentDate());
            msg.AimId_ = Styling::getTryOnAccount();
            msg.SetOutgoing(i.senderFriendly_.isEmpty());
            auto item = ComplexMessageItemBuilder::makeComplexItem(this, msg, ComplexMessageItemBuilder::ForcePreview::No);

            item->setEdited(i.isEdited_);
            item->setTime(QDateTime(QDate::currentDate(), i.time_).toTime_t());
            item->setHasAvatar(false);
            item->setMchatSender(i.senderFriendly_);
            item->setHasSenderName(!item->isOutgoing());
            item->setContextMenuEnabled(false);
            item->setMultiselectEnabled(false);

            item->onActivityChanged(true);

            item->setAttribute(Qt::WA_TransparentForMouseEvents);

            pageLayout->addWidget(item.release());
        }

        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::wallpaperImageAvailable, this, &WallpaperPreviewWidget::onWallpaperAvailable);
        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::chatFontParamsChanged, this, &WallpaperPreviewWidget::onFontParamsChanged);
        connect(&Styling::getThemesContainer(), &Styling::ThemesContainer::globalThemeChanged, this, &WallpaperPreviewWidget::onGlobalThemeChanged);
    }

    void WallpaperPreviewWidget::updateFor(const QString& _aimId)
    {
        if (aimId_ != _aimId)
            aimId_ = _aimId;

        bg_->updateWallpaper(aimId_);
    }

    void WallpaperPreviewWidget::resizeEvent(QResizeEvent *)
    {
        onResize();
    }

    void WallpaperPreviewWidget::showEvent(QShowEvent *)
    {
        onResize();
    }

    void WallpaperPreviewWidget::onWallpaperAvailable(const Styling::WallpaperId& _id)
    {
        const auto wallpaperId = Styling::getThemesContainer().getContactWallpaperId(aimId_);
        if (wallpaperId == _id)
            updateFor(aimId_);
    }

    void WallpaperPreviewWidget::onFontParamsChanged()
    {
        foreachMessage(this, [](const auto msg) { msg->updateFonts(); });
    }

    void WallpaperPreviewWidget::onGlobalThemeChanged()
    {
        updateFor(aimId_);
    }

    void WallpaperPreviewWidget::onResize()
    {
        const auto w = width();
        foreachMessage(this, [w](const auto msg)
        {
            msg->setGeometry(msg->pos().x(), msg->pos().y(), w, msg->height());
            msg->updateSize();
            msg->update();
        });
    }
}
