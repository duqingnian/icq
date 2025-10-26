#include "stdafx.h"

#include "GalleryList.h"
#include "ImageVideoList.h"
#include "LinkList.h"
#include "FilesList.h"
#include "PttList.h"

#include "SidebarUtils.h"

#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../utils/stat_utils.h"
#include "../../core_dispatcher.h"
#include "../GroupChatOperations.h"
#include "../../controls/TextUnit.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../controls/TooltipWidget.h"
#include "../containers/FriendlyContainer.h"
#include "../contact_list/ContactListModel.h"
#include "../contact_list/FavoritesUtils.h"
#include "previewer/toast.h"
#include "../MainWindow.h"

namespace
{
    constexpr int PAGE_SIZE = 20;

    QMap<QString, QVariant> makeData(const QString& _command, qint64 _msg, const QString& _link, const QString& _sender, time_t _time)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("msg")] = _msg;
        result[qsl("link")] = _link;
        result[qsl("sender")] = _sender;
        result[qsl("time")] = (qlonglong)_time;
        return result;
    }

    core::stats::stats_event_names event_by_type(Ui::MediaContentType _type)
    {
        switch (_type)
        {
        case Ui::MediaContentType::ImageVideo:
            return core::stats::stats_event_names::chatgalleryscr_phmenu_action;

        case Ui::MediaContentType::Video:
            return core::stats::stats_event_names::chatgalleryscr_vidmenu_action;

        case Ui::MediaContentType::Files:
            return core::stats::stats_event_names::chatgalleryscr_filemenu_action;

        case Ui::MediaContentType::Links:
            return core::stats::stats_event_names::chatgalleryscr_linkmenu_action;

        case Ui::MediaContentType::Voice:
            return core::stats::stats_event_names::chatgalleryscr_pttmenu_action;

        default:
            im_assert(false);
        }

        return core::stats::stats_event_names::chatgalleryscr_filemenu_action;
    }
}

namespace Ui
{
    QString getGalleryTitle(MediaContentType _type)
    {
        switch (_type)
        {
        case MediaContentType::ImageVideo:
            return (QT_TRANSLATE_NOOP("sidebar", "Photo and video"));
        case MediaContentType::Video:
            return (QT_TRANSLATE_NOOP("sidebar", "Video"));
        case MediaContentType::Files:
            return (QT_TRANSLATE_NOOP("sidebar", "Files"));
        case MediaContentType::Links:
            return (QT_TRANSLATE_NOOP("sidebar", "Links"));
        case MediaContentType::Voice:
            return (QT_TRANSLATE_NOOP("sidebar", "Voice messages"));
        default:
            im_assert(false);
        }
        return QString();
    }

    int countForType(const Data::DialogGalleryState& _state, MediaContentType _type)
    {
        switch (_type)
        {
        case MediaContentType::ImageVideo:
            return _state.imagesCount_ + _state.videosCount_;
        case MediaContentType::Video:
            return _state.videosCount_;
        case MediaContentType::Files:
            return _state.filesCount_;
        case MediaContentType::Links:
            return _state.linksCount_;
        case MediaContentType::Voice:
            return _state.pttCount_;
        default:
            break;
        }

        return 0;
    }

    void MediaContentWidget::onMenuAction(QAction *_action)
    {
        const auto params = _action->data().toMap();
        const auto command = params[qsl("command")].toString();
        const auto link = params[qsl("link")].toString();
        const auto msg = params[qsl("msg")].toLongLong();
        const auto sender = params[qsl("sender")].toString();
        const auto time = params[qsl("time")].toLongLong();

        auto makeQuote = [this, &sender, &link, &msg, &time]()
        {
            Data::Quote quote;
            quote.chatId_ = aimId_;
            quote.senderId_ = sender;
            quote.text_ = link;
            quote.msgId_ = msg;
            quote.time_ = time;
            quote.senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(sender);
            return quote;
        };

        if (!msg)
            return;

        core::stats::event_props_type props;

        if (command == u"go_to")
        {
            const auto openedAs = Utils::InterConnector::instance().getMainWindow()->isFeedAppPage() ? PageOpenedAs::FeedPage : PageOpenedAs::MainPage;
            Q_EMIT Logic::getContactListModel()->select(aimId_, msg, Logic::UpdateChatSelection::No, false, openedAs);
            props.emplace_back("Event", "Go_to_message");

            Ui::GetDispatcher()->post_stats_to_core(event_by_type(type_), { { "chat_type", Utils::chatTypeByAimId(aimId_) }, { "do", "go_to_message" } });
        }
        else if (command == u"forward")
        {
            Ui::forwardMessage({ makeQuote() }, false, QString(), QString(), !Favorites::isFavorites(aimId_));
            props.emplace_back("Event", "Forward");

            Ui::GetDispatcher()->post_stats_to_core(event_by_type(type_), { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "forward" } });
        }
        else if (command == u"add_to_favorites")
        {
            Favorites::addToFavorites({ makeQuote() });
            Favorites::showSavedToast();
        }
        else if (command == u"copy_link")
        {
            if (!link.isEmpty())
                QApplication::clipboard()->setText(link);

            props.emplace_back("Event", "Copy");

            Ui::GetDispatcher()->post_stats_to_core(event_by_type(type_), { { "chat_type", Utils::chatTypeByAimId(aimId_) }, { "do", "copy" } });
        }
        else if (const auto is_shared = command == u"delete_all"; is_shared || command == u"delete")
        {
            QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete message?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Delete message"),
                nullptr
            );

            props.emplace_back("Event", (is_shared ? "Remove_from_all" : "Remove_from_myself"));

            if (confirm)
                GetDispatcher()->deleteMessages(aimId_, { DeleteMessageInfo(msg, QString(), is_shared) });
        }

        if (props.size())
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatgalleryscr_media_event, props);
    }

    void MediaContentWidget::mousePressEvent(QMouseEvent *_event)
    {
        pos_ = _event->pos();
    }

    void MediaContentWidget::mouseReleaseEvent(QMouseEvent *_event)
    {
        if (Utils::clicked(pos_, _event->pos()))
        {
            auto item = itemAt(_event->pos());
            if (item.isNull())
                return;

            if (_event->button() == Qt::LeftButton)
                onClicked(item);
            else
                showContextMenu(item, _event->pos());
        }
    }

    void MediaContentWidget::showContextMenu(ItemData _data, const QPoint& _pos, bool _inverted)
    {
        auto menu = makeContextMenu(_data.msg_, _data.link_, _data.sender_, _data.time_, aimId_);

        connect(menu, &ContextMenu::triggered, this, &MediaContentWidget::onMenuAction, Qt::QueuedConnection);
        connect(menu, &ContextMenu::triggered, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);
        connect(menu, &ContextMenu::aboutToHide, menu, &ContextMenu::deleteLater, Qt::QueuedConnection);

        menu->showAtLeft(_inverted);

        menu->popup(mapToGlobal(_pos));
    }

    bool MediaContentWidget::isTooltipActivated() const
    {
        return tooltipActivated_;
    }

    void MediaContentWidget::showTooltip(QString _text, QRect _rect, Tooltip::ArrowDirection _arrowDir, Tooltip::ArrowPointPos _arrowPos)
    {
        hideTooltip();

        if (!tooltipTimer_)
        {
            tooltipTimer_ = new QTimer(this);
            tooltipTimer_->setInterval(Tooltip::getDefaultShowDelay());
            tooltipTimer_->setSingleShot(true);
        }
        else
        {
            tooltipTimer_->disconnect(this);
        }


        connect(tooltipTimer_, &QTimer::timeout, this, [text = std::move(_text), _rect, _arrowDir, _arrowPos]()
        {
            Tooltip::show(text, _rect, {}, _arrowDir, _arrowPos, TextRendering::HorAligment::LEFT, {}, Tooltip::TooltipMode::Multiline);
        });
        tooltipTimer_->start();

        tooltipActivated_ = true;
    }

    void MediaContentWidget::hideTooltip()
    {
        if (tooltipTimer_)
            tooltipTimer_->stop();

        Tooltip::hide();

        tooltipActivated_ = false;
    }

    ContextMenu *MediaContentWidget::makeContextMenu(qint64 _msg, const QString &_link, const QString& _sender, time_t _time, const QString& _aimid)
    {
        const auto showTrustedActions =
            type_ != MediaContentType::Files ||
            !Logic::getContactListModel()->isTrustRequired(_aimid);

        auto menu = new ContextMenu(this);

        menu->addActionWithIcon(qsl(":/context_menu/goto"), QT_TRANSLATE_NOOP("gallery", "Go to message"), makeData(qsl("go_to"), _msg, _link, _sender, _time));

        if (showTrustedActions)
            menu->addActionWithIcon(qsl(":/context_menu/forward"), QT_TRANSLATE_NOOP("context_menu", "Forward"), makeData(qsl("forward"), _msg, _link, _sender, _time));

        if (type_ == MediaContentType::Links)
            menu->addActionWithIcon(qsl(":/context_menu/link"), QT_TRANSLATE_NOOP("context_menu", "Copy link"), makeData(qsl("copy_link"), _msg, _link, _sender, _time));

        if (showTrustedActions && !Favorites::isFavorites(_aimid))
        {
            menu->addSeparator();
            menu->addActionWithIcon(qsl(":/context_menu/favorites"), QT_TRANSLATE_NOOP("context_menu", "Add to favorites"), makeData(qsl("add_to_favorites"), _msg, _link, _sender, _time));
        }

        return menu;
    }

    GalleryList::GalleryList(QWidget* _parent, const QString& _type, MediaContentWidget* _contentWidget)
        : GalleryList(_parent, QStringList() << _type, _contentWidget)
    {
    }

    GalleryList::GalleryList(QWidget* _parent, const QStringList& _types, MediaContentWidget* _contentWidget)
        : QWidget(_parent)
        , contentWidget_(_contentWidget)
        , types_(_types)
        , lastMsgId_(0)
        , lastSeq_(0)
        , requestId_(-1)
        , exhausted_(false)
    {
        area_ = CreateScrollAreaAndSetTrScrollBarV(this);
        area_->setStyleSheet(qsl("background: transparent;"));
        area_->setWidget(contentWidget_ ? contentWidget_ : new QWidget(this));
        area_->setWidgetResizable(true);
        area_->setFocusPolicy(Qt::NoFocus);
        area_->setFrameShape(QFrame::NoFrame);
        Testing::setAccessibleName(area_, qsl("AS GalleryPage scrollArea"));

        auto vLayout = Utils::emptyVLayout(this);
        vLayout->addWidget(area_);

        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryResult, this, &GalleryList::dialogGalleryResult);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryUpdate, this, &GalleryList::dialogGalleryUpdate);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dialogGalleryHolesDownloaded, this, &GalleryList::dialogGalleryHolesDownloaded);
        connect(area_->verticalScrollBar(), &QScrollBar::valueChanged, this, &GalleryList::scrolled, Qt::QueuedConnection);
    }

    GalleryList::~GalleryList() = default;

    void GalleryList::initFor(const QString& _aimId)
    {
        if (!contentWidget_)
            return;

        aimId_ = _aimId;
        contentWidget_->initFor(_aimId);
        lastMsgId_ = 0;
        lastSeq_ = 0;
        exhausted_ = false;

        requestId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, types_, lastMsgId_, lastSeq_, PAGE_SIZE, true);
    }

    void GalleryList::markClosed()
    {
        aimId_.clear();

        if (contentWidget_)
            contentWidget_->markClosed();
    }

    void GalleryList::setContentWidget(MediaContentWidget* _contentWidget)
    {
        markClosed();

        area_->setWidget(_contentWidget);
        contentWidget_ = _contentWidget;
    }

    void GalleryList::setType(const QString& _type)
    {
        types_.clear();
        types_ << _type;
    }

    void GalleryList::setTypes(QStringList _types)
    {
        types_ = std::move(_types);
    }

    void GalleryList::openContentFor(const QString& _aimId, MediaContentType _type)
    {
        openContentForType(_type);
        initFor(_aimId);
    }

    void GalleryList::openContentForType(MediaContentType _type)
    {
        switch (_type)
        {
        case MediaContentType::ImageVideo:
            setTypes({ qsl("image"), qsl("video") });
            setContentWidget(new ImageVideoList(this, MediaContentType::ImageVideo, "Photo+Video"));
            Testing::setAccessibleName(contentWidget_, qsl("AS GalleryList imageVideoList"));
            break;

        case MediaContentType::Video:
            setType(qsl("video"));
            setContentWidget(new ImageVideoList(this, MediaContentType::Video, "Video"));
            Testing::setAccessibleName(contentWidget_, qsl("AS GalleryList videoList"));
            break;

        case MediaContentType::Files:
            setTypes({ qsl("file"), qsl("audio") });
            setContentWidget(new FilesList(this));
            Testing::setAccessibleName(contentWidget_, qsl("AS GalleryList filesList"));
            break;

        case MediaContentType::Links:
            setType(qsl("link"));
            setContentWidget(new LinkList(this));
            Testing::setAccessibleName(contentWidget_, qsl("AS GalleryList linksList"));
            break;

        case MediaContentType::Voice:
            setType(qsl("ptt"));
            setContentWidget(new PttList(this));
            Testing::setAccessibleName(contentWidget_, qsl("AS GalleryList pttList"));
            break;

        default:
            im_assert(false);
        }
    }

    void GalleryList::setMaxContentWidth(int _width)
    {
        area_->setMaxContentWidth(_width);
    }

    const QString& GalleryList::currentAimId() const
    {
        return aimId_;
    }

    bool GalleryList::exhausted() const
    {
        return exhausted_;
    }

    void GalleryList::resizeEvent(QResizeEvent *_event)
    {
        if (contentWidget_)
        {
            if (!exhausted() && !aimId_.isEmpty() && requestId_ == -1 && contentWidget_->height() <= area_->height())
                requestId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, types_, lastMsgId_, lastSeq_, PAGE_SIZE, true);
        }

        QWidget::resizeEvent(_event);
    }

    void GalleryList::dialogGalleryResult(const int64_t _seq, const QVector<Data::DialogGalleryEntry>& _entries, bool _exhausted)
    {
        if (!contentWidget_ || requestId_ != _seq)
            return;

        requestId_ = -1;
        exhausted_ = _exhausted;
        if (!_entries.empty())
        {
            lastMsgId_ = _entries.back().msg_id_;
            lastSeq_ = _entries.back().seq_;
        }

        contentWidget_->processItems(_entries);

        if (exhausted() || _entries.empty())
            return;

        if(contentWidget_->height() <= area_->height() && !aimId_.isEmpty())
            requestId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, types_, lastMsgId_, lastSeq_, PAGE_SIZE, true);
    }

    void GalleryList::dialogGalleryUpdate(const QString& _aimid, const QVector<Data::DialogGalleryEntry>& _entries)
    {
        if (!contentWidget_ || aimId_ != _aimid)
            return;

        QVector<Data::DialogGalleryEntry> updates;
        updates.reserve(_entries.size());
        std::copy_if(
            _entries.begin(),
            _entries.end(),
            std::back_inserter(updates),
            [this](const auto& e) { return types_.contains(e.type_) || e.action_ == u"del"; });
        if (!updates.isEmpty())
            contentWidget_->processUpdates(updates);
    }

    void GalleryList::dialogGalleryHolesDownloaded(const QString& _aimid)
    {
        if (!contentWidget_ || aimId_ != _aimid)
            return;

        if (exhausted())
            return;

        if (contentWidget_->height() <= area_->height() && !aimId_.isEmpty())
            requestId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, types_, lastMsgId_, lastSeq_, PAGE_SIZE, true);
    }

    void GalleryList::scrolled(int value)
    {
        if (!contentWidget_)
            return;

        contentWidget_->scrolled();

        auto scroll = area_->verticalScrollBar();
        if (exhausted())
            return;

        if (value > scroll->maximum() * 0.7 && requestId_ == -1 && !aimId_.isEmpty())
        {
            requestId_ = Ui::GetDispatcher()->getDialogGallery(aimId_, types_, lastMsgId_, lastSeq_, PAGE_SIZE, true);
        }
    }

}
