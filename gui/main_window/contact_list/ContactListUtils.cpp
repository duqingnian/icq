#include <utils/features.h>
#include "stdafx.h"

#include "ContactListUtils.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "UnknownsModel.h"
#include "SearchModel.h"
#include "CommonChatsModel.h"
#include "Common.h"
#include "../containers/FriendlyContainer.h"

#include "../ReportWidget.h"
#include "../GroupChatOperations.h"
#include "../../core_dispatcher.h"
#include "../../utils/InterConnector.h"
#include "ServiceContacts.h"

#include "../../controls/CustomButton.h"
#include "../../controls/TextWidget.h"

#include "styles/ThemeParameters.h"

#include "IgnoreMembersModel.h"
#include "controls/ContextMenu.h"
#include "main_window/contact_list/FavoritesUtils.h"

namespace Logic
{
    bool is_members_regim(int _regim)
    {
        return
        _regim == Logic::MembersWidgetRegim::MEMBERS_LIST ||
        _regim == Logic::MembersWidgetRegim::IGNORE_LIST ||
        _regim == Logic::MembersWidgetRegim::DISALLOWED_INVITERS;
    }

    bool is_admin_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::ADMIN_MEMBERS;
    }

    bool is_select_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS || _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE
            || _regim == Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS;
    }

    bool is_select_chat_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS;
    }

    bool is_video_conference_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    bool is_share_regims(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SHARE || _regim == Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE;
    }

    bool isRegimWithGlobalContactSearch(int _regim)
    {
        constexpr Logic::MembersWidgetRegim regimes[] =
        {
            Logic::MembersWidgetRegim::SHARE,
            Logic::MembersWidgetRegim::SHARE_VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::SELECT_MEMBERS,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS,
            Logic::MembersWidgetRegim::SHARE_CONTACT,
            Logic::MembersWidgetRegim::TASK_ASSIGNEE,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
        };
        return std::any_of(std::begin(regimes), std::end(regimes), [_regim](auto r){ return r == _regim; });
    }

    bool isAddMembersRegim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::SELECT_MEMBERS || _regim == Logic::MembersWidgetRegim::VIDEO_CONFERENCE;
    }

    QString aimIdFromIndex(const QModelIndex& _index)
    {
        QString aimid;
        if (const auto searchRes = _index.data().value<Data::AbstractSearchResultSptr>())
        {
            aimid = searchRes->getAimId();
        }
        else if (auto cont = _index.data().value<Data::Contact*>())
        {
            aimid = cont->AimId_;
        }
        else if (const auto memberInfo = _index.data().value<Data::ChatMemberInfo*>())
        {
            aimid = memberInfo->AimId_;
        }
        else if (qobject_cast<const Logic::CommonChatsModel*>(_index.model()))
        {
            Data::CommonChatInfo ccInfo = _index.data().value<Data::CommonChatInfo>();
            aimid = ccInfo.aimid_;
        }
        else if (qobject_cast<const Logic::UnknownsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else if (qobject_cast<const Logic::RecentsModel*>(_index.model()))
        {
            Data::DlgState dlg = _index.data().value<Data::DlgState>();
            aimid = dlg.AimId_;
        }
        else if (auto callInfo = _index.data().value<Data::CallInfoPtr>())
        {
            aimid = callInfo->getAimId();
        }

        if (aimid.startsWith(ql1c('~')) && ServiceContacts::contactType(aimid) != ServiceContacts::ContactType::ThreadsFeed)
            return QString();

        return aimid;
    }

    QString senderAimIdFromIndex(const QModelIndex& _index)
    {
        if (const auto searchRes = _index.data().value<Data::AbstractSearchResultSptr>())
            return searchRes->getSenderAimId();

        return aimIdFromIndex(_index);
    }

    QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("contact")] = _aimid;
        return result;
    }

    QMap<QString, QVariant> makeData(const QString& _command, Data::CallInfoPtr _call)
    {
        QMap<QString, QVariant> result;
        result[qsl("command")] = _command;
        result[qsl("call")] = QVariant::fromValue(_call);
        return result;
    }

    void addItemToContextMenu(QMenu* _popupMenu, const QString& _aimId, const Data::DlgState& _dlg)
    {
        auto popupMenu = qobject_cast<Ui::ContextMenu *>(_popupMenu);
        if (_dlg.UnreadCount_ != 0 || _dlg.Attention_)
            popupMenu->addActionWithIcon(qsl(":/context_menu/mark_read"), QT_TRANSLATE_NOOP("context_menu", "Mark as read"), Logic::makeData(qsl("recents/mark_read"), _aimId));
        else if (!_dlg.Attention_)
            popupMenu->addActionWithIcon(qsl(":/context_menu/mark_unread"), QT_TRANSLATE_NOOP("context_menu", "Mark as unread"), Logic::makeData(qsl("recents/mark_unread"), _aimId));

        if (!Logic::getRecentsModel()->isUnimportant(_aimId))
        {
            if (Logic::getRecentsModel()->isFavorite(_aimId))
                popupMenu->addActionWithIcon(qsl(":/context_menu/unpin"), QT_TRANSLATE_NOOP("context_menu", "Unpin"), Logic::makeData(qsl("recents/unfavorite"), _aimId));
            else
                popupMenu->addActionWithIcon(qsl(":/context_menu/pin"), QT_TRANSLATE_NOOP("context_menu", "Pin"), Logic::makeData(qsl("recents/favorite"), _aimId));
        }

        if (!Logic::getRecentsModel()->isFavorite(_aimId))
        {
            if (Logic::getRecentsModel()->isUnimportant(_aimId))
                popupMenu->addActionWithIcon(qsl(":/context_menu/unmark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Remove from Unimportant"), Logic::makeData(qsl("recents/remove_from_unimportant"), _aimId));
            else
                popupMenu->addActionWithIcon(qsl(":/context_menu/mark_unimportant"), QT_TRANSLATE_NOOP("context_menu", "Move to Unimportant"), Logic::makeData(qsl("recents/mark_unimportant"), _aimId));
        }

        const auto isNewsFeed = Logic::getContactListModel()->isFeed(_aimId);
        const auto canLeaveAndReport = !isNewsFeed || (isNewsFeed && Logic::getContactListModel()->areYouAdmin(_aimId));

        if (!Favorites::isFavorites(_aimId))
        {
            if (Logic::getContactListModel()->contains(_aimId))
            {
                if (Logic::getContactListModel()->isMuted(_aimId))
                    popupMenu->addActionWithIcon(qsl(":/context_menu/mute_off"), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), Logic::makeData(qsl("recents/unmute"), _aimId));
                else
                    popupMenu->addActionWithIcon(qsl(":/context_menu/mute"), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), Logic::makeData(qsl("recents/mute"), _aimId));
            }

            if (canLeaveAndReport)
            {
                popupMenu->addSeparator();

                popupMenu->addActionWithIcon(qsl(":/clear_chat_icon"), QT_TRANSLATE_NOOP("context_menu", "Clear history"), Logic::makeData(qsl("recents/clear_history"),_aimId));


                if (Logic::getIgnoreModel()->contains(_aimId))
                    popupMenu->addActionWithIcon(qsl(":/context_menu/unblock"), QT_TRANSLATE_NOOP("context_menu", "Unblock"), Logic::makeData(qsl("recents/unblock"), _aimId));
                else
                    popupMenu->addActionWithIcon(qsl(":/ignore_icon"), QT_TRANSLATE_NOOP("context_menu", "Block"), Logic::makeData(qsl("recents/ignore"), _aimId));

                if (Logic::getContactListModel()->isChat(_aimId))
                {
                    if (Features::isReportMessagesEnabled())
                        popupMenu->addActionWithIcon(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("context_menu", "Report and block"), Logic::makeData(qsl("recents/report"), _aimId));

                    popupMenu->addActionWithIcon(qsl(":/exit_icon"), QT_TRANSLATE_NOOP("context_menu", "Leave and delete"), Logic::makeData(qsl("contacts/remove"), _aimId));
                }
                else if (Features::clRemoveContactsAllowed())
                {
                    popupMenu->addActionWithIcon(qsl(":/alert_icon"), QT_TRANSLATE_NOOP("context_menu", "Report"), Logic::makeData(qsl("recents/report"), _aimId));
                }
            }
        }

        if (!Logic::getRecentsModel()->isFavorite(_aimId) && !Logic::getRecentsModel()->isUnimportant(_aimId))
        {
            if (Features::clRemoveContactsAllowed() && !Logic::getContactListModel()->isChat(_aimId) && !Favorites::isFavorites(_aimId) && canLeaveAndReport)
                popupMenu->addActionWithIcon(qsl(":/context_menu/delete"), QT_TRANSLATE_NOOP("context_menu", "Remove"), Logic::makeData(qsl("recents/remove"), _aimId));

            popupMenu->addSeparator();
            popupMenu->addActionWithIcon(qsl(":/context_menu/close"), QT_TRANSLATE_NOOP("context_menu", "Hide"), Logic::makeData(qsl("recents/close"), _aimId));
        }

        popupMenu->popup(QCursor::pos());
    }

    void showContactListPopup(QAction* _action)
    {
        const auto params = _action->data().toMap();
        const QString command = params[qsl("command")].toString();
        const QString aimId = params[qsl("contact")].toString();
        Data::CallInfoPtr call = params[qsl("call")].value<Data::CallInfoPtr>();

        if (command == u"recents/mark_read")
        {
            const auto dlgState = Logic::getRecentsModel()->getDlgState(aimId);
            if (dlgState.Attention_)
            {
                Logic::getRecentsModel()->setAttention(aimId, false);
            }
            else
            {
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_read);
                Logic::getRecentsModel()->sendLastRead(aimId, true, Logic::RecentsModel::ReadMode::ReadAll);
            }
        }
        else if (command == u"recents/mute")
        {
            Logic::getRecentsModel()->muteChat(aimId, true);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mute_recents_menu);
        }
        else if (command == u"recents/unmute")
        {
            Logic::getRecentsModel()->muteChat(aimId, false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unmute);
        }
        else if (command == u"contacts/ignore")
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "ContactList_Menu" } });
        }
        else if (command == u"recents/ignore")
        {
            if (Logic::getContactListModel()->ignoreContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::ignore_contact, { { "From", "Recents_Menu" } });
        }
        else if (command == u"recents/unblock")
        {
            if (Logic::getContactListModel()->unblockContactWithConfirm(aimId))
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::chatscr_unblockuser_action, { {"From", "Recents_Menu" } });
        }
        else if (command == u"recents/close")
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_close);
            Logic::getRecentsModel()->hideChat(aimId);
        }
        else if (command == u"recents/remove")
        {
            const QString text = QT_TRANSLATE_NOOP("popup_window", "Remove %1 from your contacts?").arg(Logic::GetFriendlyContainer()->getFriendly(aimId));

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                QT_TRANSLATE_NOOP("popup_window", "Remove contact"),
                nullptr
            );

            if (confirm)
            {
                Logic::getRecentsModel()->hideChat(aimId);
                Logic::getContactListModel()->removeContactFromCL(aimId);
            }
        }
        else if (command == u"recents/mark_unread")
        {
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mark_unread);
            if (Logic::getContactListModel()->selectedContact() == aimId)
            {
                Q_EMIT Logic::getContactListModel()->leave_dialog(aimId);
                Utils::InterConnector::instance().closeDialog();
            }
            Logic::getRecentsModel()->setAttention(aimId, true);
        }
        else if (command == u"contacts/call")
        {
#ifndef STRIP_VOIP
            Ui::GetDispatcher()->getVoipController().setStartCall({ aimId }, true, false);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::call_audio, { { "From", "ContactList_Menu" } });
#endif
        }
        else if (command == u"contacts/Profile")
        {
            Q_EMIT Utils::InterConnector::instance().profileSettingsShow(aimId);
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_open, { { "From", "CL_Popup" } });
        }
        else if (command == u"contacts/spam")
        {
            if (Ui::ReportContact(aimId, Logic::GetFriendlyContainer()->getFriendly(aimId)))
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "ContactList_Menu" } });
            }
        }
        else if (command == u"contacts/remove")
        {
            const auto isChat = Utils::isChat(aimId);

            QString text;
            if (isChat)
            {
                text = Logic::getContactListModel()->isChannel(aimId)
                   ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave channel?")
                   : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to erase history and leave chat?");
            }
            else
            {
                text = QT_TRANSLATE_NOOP("popup_window", "Remove %1 from your contacts?").arg(Logic::GetFriendlyContainer()->getFriendly(aimId));
            }

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                isChat ? QT_TRANSLATE_NOOP("popup_window", "Leave") : QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                isChat ? QT_TRANSLATE_NOOP("sidebar", "Leave and delete") : QT_TRANSLATE_NOOP("popup_window", "Remove contact"),
                nullptr
            );

            if (confirm)
            {
                Logic::getContactListModel()->removeContactFromCL(aimId);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_contact, { { "From", "ContactList_Menu" } });
            }
        }
        else if (command == u"recents/favorite")
        {
            Ui::GetDispatcher()->pinContact(aimId, true);
        }
        else if (command == u"recents/unfavorite")
        {
            Ui::GetDispatcher()->pinContact(aimId, false);
        }
        else if (command == u"recents/mark_unimportant")
        {
            const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to mark the chat as unimportant? The chat won't jump to the top of the list when you receive a new message"),
                Logic::GetFriendlyContainer()->getFriendly(aimId),
                nullptr);

            if (confirmed)
            {
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", aimId);
                Ui::GetDispatcher()->post_message_to_core("mark_unimportant", collection.get());
            }
        }
        else if (command == u"recents/remove_from_unimportant")
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core("remove_from_unimportant", collection.get());
        }
        else if (command == u"recents/clear_history")
        {
            const auto msg = Logic::getContactListModel()->isChannel(aimId)
                             ? QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to clear history?")
                             : QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to clear the chat history?");
            const auto confirmed = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                msg,
                Logic::GetFriendlyContainer()->getFriendly(aimId),
                nullptr);

            if (confirmed)
                Ui::eraseHistory(aimId);
        }
        else if (command == u"recents/report")
        {
            if (Ui::ReportContact(aimId, Logic::GetFriendlyContainer()->getFriendly(aimId)))
            {
                Logic::getContactListModel()->ignoreContact(aimId, true);
                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_contact, { { "From", "ContactList_Menu" } });
            }
        }
        else if (command == u"calls/remove")
        {
            const QString text = QT_TRANSLATE_NOOP("popup_window", "Are you sure you want to delete the call?");

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                call->getFriendly(),
                nullptr
            );

            if (confirm)
            {
                for (const auto& msg : call->getMessages())
                {
                    Ui::DeleteMessageInfo info(msg->Id_, QString(), false);
                    Ui::GetDispatcher()->deleteMessages(msg->AimId_, { info });
                }
            }
        }
    }
}

namespace
{
    int emptyLabelHorMargin() noexcept
    {
        return Utils::scale_value(16);
    }

    auto btnHeight() noexcept
    {
        return Utils::scale_value(30);
    }

    auto btnMaxWidth() noexcept
    {
        return Utils::scale_value(256);
    }

    auto leftMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto rightMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    constexpr QSize btnSizeUnscaled() noexcept
    {
        return QSize(14, 14);
    }

    auto headerViewHeight() noexcept
    {
        return Utils::scale_value(44); // 10 + 24 + 10
    }

    auto categoryMargin() noexcept
    {
        return Utils::scale_value(12);
    }

    auto categorySpacing() noexcept
    {
        return Utils::scale_value(8);
    }

    auto searchFilterBtnColorKey()
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::BASE_GLOBALWHITE };
    }

    auto topSearchFilterBtnTextOffset() noexcept
    {
        return Utils::scale_value(2);
    }

    auto bottomSearchFilterBtnTextOffset() noexcept
    {
        return Utils::scale_value(2);
    }

    auto offsetIconSearchFilterBtn() noexcept
    {
        return Utils::scale_value(8);
    }

    auto iconSearchRightOffset() noexcept
    {
        return Utils::scale_value(8);
    }

    auto radiusSearchFilterBtn() noexcept
    {
        return Utils::scale_value(btnHeight() / 2);
    }

    QSize sizeFullIconSearchFilterBtnUnscaled() noexcept
    {
        return QSize(btnSizeUnscaled().width() + offsetIconSearchFilterBtn() + iconSearchRightOffset(),
                     Utils::unscale_value(btnHeight()));
    }

    auto getCategoryFont()
    {
        return Fonts::appFontScaled(15, Fonts::FontWeight::SemiBold);
    }
}

namespace Ui
{
    EmptyListLabel::EmptyListLabel(QWidget* _parent, Logic::MembersWidgetRegim _regim)
        : LinkAccessibleWidget(_parent)
    {
        QString text;
        if (_regim == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
        {
            text =
                QT_TRANSLATE_NOOP("settings", "Specify who should be prohibited from adding you to groups")
                % QChar::LineFeed
                % QChar::LineFeed
                % QT_TRANSLATE_NOOP("settings", "Add to list");
        }
        else if (_regim == Logic::MembersWidgetRegim::IGNORE_LIST)
        {
            text = QT_TRANSLATE_NOOP("sidebar", "You have no ignored contacts");
        }
        else
        {
            text = QT_TRANSLATE_NOOP("sidebar", "List empty");
        }

        label_ = TextRendering::MakeTextUnit(text, {});
        TextRendering::TextUnit::InitializeParameters params(Fonts::appFontScaled(16), Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID });
        params.linkColor_ = Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
        params.align_ = TextRendering::HorAligment::CENTER;
        label_->init(params);

        if (_regim == Logic::MembersWidgetRegim::DISALLOWED_INVITERS)
            label_->applyLinks({ { QT_TRANSLATE_NOOP("settings", "Add to list"), QString() } });


        label_->evaluateDesiredSize();
        setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        setMouseTracking(true);
    }

    EmptyListLabel::~EmptyListLabel() = default;

    void EmptyListLabel::resizeEvent(QResizeEvent* _e)
    {
        if (width() != _e->oldSize().width())
        {
            const auto newW = width() - emptyLabelHorMargin() * 2;
            const auto h = label_->getHeight(newW);
            setFixedHeight(std::max(Utils::scale_value(40), h));

            label_->setOffsets((width() - newW) / 2, 0);
        }
    }

    void EmptyListLabel::paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        label_->draw(p);
    }

    void EmptyListLabel::mouseMoveEvent(QMouseEvent* _event)
    {
        setLinkHighlighted(label_->isOverLink(_event->pos()));
        update();
    }

    void EmptyListLabel::mouseReleaseEvent(QMouseEvent* _event)
    {
        if (_event->button() == Qt::LeftButton && label_->isOverLink(_event->pos()))
            Q_EMIT Utils::InterConnector::instance().showAddToDisallowedInvitersDialog();
    }

    void EmptyListLabel::leaveEvent(QEvent*)
    {
        setLinkHighlighted(false);
        update();
    }

    void EmptyListLabel::setLinkHighlighted(bool _hl)
    {
        setCursor(_hl ? Qt::PointingHandCursor : Qt::ArrowCursor);
        label_->setLinkColor(Styling::ThemeColorKey{ _hl ? Styling::StyleVariable::TEXT_PRIMARY_HOVER : Styling::StyleVariable::TEXT_PRIMARY });
    }

    SearchCategoryButton::SearchCategoryButton(QWidget* _parent, const QString& _text)
        : ClickableWidget(_parent)
        , selected_(false)
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);

        label_ = TextRendering::MakeTextUnit(_text, {}, TextRendering::LinksVisible::DONT_SHOW_LINKS, TextRendering::ProcessLineFeeds::REMOVE_LINE_FEEDS);

        label_->init({ getCategoryFont(), getTextColor() });
        label_->setOffsets(leftMargin(), (btnHeight() - topSearchFilterBtnTextOffset()) / 2);
        label_->evaluateDesiredSize();

        setFixedSize(label_->desiredWidth() + leftMargin() + rightMargin(), btnHeight());
        connect(this, &SearchCategoryButton::hoverChanged, this, qOverload<>(&SearchCategoryButton::update));
        update();
    }

    SearchCategoryButton::~SearchCategoryButton() = default;

    bool SearchCategoryButton::isSelected() const
    {
        return selected_;
    }

    void SearchCategoryButton::setSelected(const bool _sel)
    {
        if (selected_ == _sel)
            return;

        selected_ = _sel;

        label_->setColor(getTextColor());

        update();
    }

    void SearchCategoryButton::paintEvent(QPaintEvent* _e)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        const auto radius = height() / 2;
        const auto backgroundColor = Styling::getParameters().getColor(Styling::StyleVariable::SECONDARY_SELECTED);
        const auto backgroundColorHovered = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT);

        if (selected_)
            p.setBrush(backgroundColor);
        else if (isHovered())
            p.setBrush(backgroundColorHovered);
        else
            p.setBrush(Qt::NoBrush);

        if (selected_ || isHovered())
            p.setPen(Qt::NoPen);
        else
            p.setPen(Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT));
        p.drawRoundedRect(rect(), radius, radius);


        if (label_)
            label_->draw(p, TextRendering::VerPosition::MIDDLE);
    }

    Styling::ThemeColorKey SearchCategoryButton::getTextColor() const
    {
        return Styling::ThemeColorKey{ selected_ ? Styling::StyleVariable::TEXT_PRIMARY : Styling::StyleVariable::BASE_PRIMARY };
    }

    HeaderScrollOverlay::HeaderScrollOverlay(QWidget* _parent, QScrollArea* _scrollArea)
        : QWidget(_parent)
        , scrollArea_(_scrollArea)
    {
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

    void HeaderScrollOverlay::paintEvent(QPaintEvent * _event)
    {
        const auto scrollWidget = scrollArea_->widget();
        const auto posWidget = mapToGlobal(scrollWidget->pos());
        const auto posViewport = mapToGlobal(pos());

        const QSize gradientSize(Utils::scale_value(32), height());

        const auto drawGradient = [this, &gradientSize](const int _left, const QColor& _start, const QColor& _end)
        {
            QRect grRect(QPoint(_left, 0), gradientSize);

            QLinearGradient gradient(grRect.topLeft(), grRect.topRight());
            gradient.setColorAt(0, _start);
            gradient.setColorAt(1, _end);

            QPainter p(this);
            p.fillRect(grRect, gradient);
        };

        const auto bg = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);

        if (posViewport.x() > posWidget.x())
            drawGradient(0, bg, Qt::transparent);

        if (posWidget.x() + scrollWidget->width() > posViewport.x() + width())
            drawGradient(width() - gradientSize.width(), Qt::transparent, bg);
    }

    bool HeaderScrollOverlay::eventFilter(QObject* _watched, QEvent* _event)
    {
        if (_watched == parentWidget() && _event->type() == QEvent::Resize)
            setGeometry(parentWidget()->geometry());

        return QWidget::eventFilter(_watched, _event);
    }

    GlobalSearchViewHeader::GlobalSearchViewHeader(QWidget* _parent)
        : QWidget(_parent)
        , animScroll_(nullptr)
    {
        setFixedHeight(headerViewHeight());

        auto rootLayout = Utils::emptyHLayout(this);

        viewArea_ = new QScrollArea(this);
        viewArea_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewArea_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        viewArea_->setFrameShape(QFrame::NoFrame);
        auto scrollAreaWidget = new QWidget(viewArea_);
        Utils::setDefaultBackground(scrollAreaWidget);
        viewArea_->setWidget(scrollAreaWidget);
        viewArea_->setWidgetResizable(true);
        viewArea_->setFocusPolicy(Qt::NoFocus);

        Testing::setAccessibleName(viewArea_, qsl("AS Search groupViewArea"));
        rootLayout->addWidget(viewArea_);

        Utils::grabTouchWidget(viewArea_->viewport(), true);
        Utils::grabTouchWidget(scrollAreaWidget);

        auto layout = Utils::emptyHLayout();
        const auto margin = categoryMargin();
        layout->setContentsMargins(margin, 0, margin, 0);
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        scrollAreaWidget->setLayout(layout);
        layout->setSpacing(categorySpacing());

        auto overlay = new HeaderScrollOverlay(this, viewArea_);
        overlay->raise();
        installEventFilter(overlay);

        const auto addCat = [layout, this](const QString& _text, const QString& _accName)
        {
            auto cat = new SearchCategoryButton(this, _text);
            Testing::setAccessibleName(cat, u"AS Search " % _accName);
            layout->addWidget(cat);

            connect(cat, &SearchCategoryButton::clicked, this, &GlobalSearchViewHeader::onCategoryClicked);

            return cat;
        };

        buttons_ =
        {
            { SearchCategory::ContactsAndGroups, addCat(QT_TRANSLATE_NOOP("search", "Contacts and groups"), qsl("contactsAndGroups")) },
            { SearchCategory::Messages, addCat(QT_TRANSLATE_NOOP("search", "Messages"), qsl("messages")) },
            { SearchCategory::Threads, addCat(QT_TRANSLATE_NOOP("search", "Threads"), qsl("threads")) },
            { SearchCategory::SingleThread, addCat(QT_TRANSLATE_NOOP("search", "Thread"), qsl("thread")) },
        };

        selectFirstVisible();
    }

    void GlobalSearchViewHeader::selectCategory(const SearchCategory _cat, const SelectType _selectType)
    {
        SearchCategory currentCat { SearchCategory::Messages };
        for (auto [cat, btn] : buttons_)
        {
            if (btn->isSelected())
            {
                currentCat = cat;
                break;
            }
        }

        const auto betweenMessagesAndThreads = (currentCat == SearchCategory::Messages && _cat == SearchCategory::Threads)
                || (currentCat == SearchCategory::Threads && _cat == SearchCategory::Messages);

        if (betweenMessagesAndThreads)
            Q_EMIT categorySelected(_cat, false);

        for (auto [cat, btn] : buttons_)
        {
            const auto isNeededCat = cat == _cat;

            if (_selectType != SelectType::Click || betweenMessagesAndThreads)
                btn->setSelected(isNeededCat);

            if (isNeededCat)
                viewArea_->ensureWidgetVisible(btn);
        }

        if (_selectType == SelectType::Click)
            Q_EMIT categorySelected(_cat);
    }

    void GlobalSearchViewHeader::setCategoryVisible(const SearchCategory _cat, const bool _isVisible)
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [_cat](const auto& it) { return it.first == _cat; });
        if (it != buttons_.end())
            it->second->setVisible(_isVisible);
    }

    void GlobalSearchViewHeader::reset()
    {
        for (auto [_, btn] : buttons_)
            btn->setVisible(true);

        selectFirstVisible();
    }

    void GlobalSearchViewHeader::selectFirstVisible()
    {
        auto it = std::find_if(buttons_.begin(), buttons_.end(), [](const auto& it) { return it.second->isVisible(); });
        if (it != buttons_.end())
            selectCategory(it->first);
    }

    bool GlobalSearchViewHeader::hasVisibleCategories() const
    {
        return std::any_of(buttons_.begin(), buttons_.end(), [](const auto& it) { return it.second->isVisible(); });
    }

    void GlobalSearchViewHeader::wheelEvent(QWheelEvent* _e)
    {
        const int numDegrees = _e->delta() / 8;
        const int numSteps = numDegrees / 15;

        if (!numSteps || !numDegrees)
            return;

        if (numSteps > 0)
            scrollStep(direction::left);
        else
            scrollStep(direction::right);

        QWidget::wheelEvent(_e);
    }

    void GlobalSearchViewHeader::scrollStep(direction _direction)
    {
        QRect viewRect = viewArea_->viewport()->geometry();
        auto scrollbar = viewArea_->horizontalScrollBar();

        const int curVal = scrollbar->value();
        const int step = viewRect.width() / 20;
        int to = 0;

        if (_direction == direction::right)
            to = std::min(curVal + step, scrollbar->maximum());
        else
            to = std::max(curVal - step, scrollbar->minimum());

        if (!animScroll_)
        {
            constexpr auto duration = std::chrono::milliseconds(300);

            animScroll_ = new QPropertyAnimation(scrollbar, QByteArrayLiteral("value"), this);
            animScroll_->setDuration(duration.count());
            animScroll_->setEasingCurve(QEasingCurve::InQuad);
        }

        animScroll_->stop();
        animScroll_->setStartValue(curVal);
        animScroll_->setEndValue(to);
        animScroll_->start();
    }

    void GlobalSearchViewHeader::onCategoryClicked()
    {
        auto newBtn = qobject_cast<SearchCategoryButton*>(sender());
        if (!newBtn)
        {
            im_assert(false);
            return;
        }

        auto it = std::find_if(buttons_.begin(), buttons_.end(), [newBtn](const auto& it) { return it.second == newBtn; });
        if (it != buttons_.end())
            selectCategory(it->first, SelectType::Click);
    }

    SearchFilterButton::SearchFilterButton(QWidget* _parent)
        : QWidget(_parent)
        , labelWidget_(new TextWidget(_parent, QString{}, Data::MentionMap(), TextRendering::LinksVisible::DONT_SHOW_LINKS))
    {
        setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
        setCursor(Qt::PointingHandCursor);

        auto layout = Utils::emptyHLayout(this);
        layout->addSpacing(leftMargin());

        labelWidget_->init({ getCategoryFont(), searchFilterBtnColorKey() });

        auto vertTextLayout = Utils::emptyVLayout();
        vertTextLayout->addStretch();
        if constexpr (platform::is_apple())
            vertTextLayout->addSpacing(topSearchFilterBtnTextOffset());
        vertTextLayout->addWidget(labelWidget_);
        if constexpr (!platform::is_apple())
            vertTextLayout->addSpacing(bottomSearchFilterBtnTextOffset());
        vertTextLayout->addStretch();

        layout->addLayout(vertTextLayout);
        layout->addStretch();

        const auto btn = new CustomButton(this, qsl(":/controls/close_icon_round"), btnSizeUnscaled(), searchFilterBtnColorKey());
        btn->setFixedSize(Utils::scale_value(sizeFullIconSearchFilterBtnUnscaled()));
        Testing::setAccessibleName(btn, qsl("AS Search deleteChatButton"));

        layout->addWidget(btn);

        connect(btn, &CustomButton::clicked, this, &SearchFilterButton::onRemoveClicked);
    }

    SearchFilterButton::~SearchFilterButton()
    {
    }

    void SearchFilterButton::setLabel(const QString& _label)
    {
        labelWidget_->setText(Data::FString(_label));

        resize(sizeHint());
        elideLabel();
        update();
    }

    QSize SearchFilterButton::sizeHint() const
    {
        return QSize(std::min(btnMaxWidth(), labelWidget_->width()), btnHeight());
    }

    void SearchFilterButton::paintEvent(QPaintEvent* _event)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        p.setBrush(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rect(), radiusSearchFilterBtn(), radiusSearchFilterBtn());
    }

    void SearchFilterButton::resizeEvent(QResizeEvent* _event)
    {
        elideLabel();
        QWidget::resizeEvent(_event);
    }

    void SearchFilterButton::elideLabel()
    {
        if (!labelWidget_->isEmpty())
            labelWidget_->elide(width() - Utils::scale_value(sizeFullIconSearchFilterBtnUnscaled().width()) - offsetIconSearchFilterBtn());
    }

    void SearchFilterButton::onRemoveClicked()
    {
        Q_EMIT removeClicked(QPrivateSignal());
    }

    DialogSearchViewHeader::DialogSearchViewHeader(QWidget* _parent)
        : QWidget(_parent)
    {
        setFixedHeight(headerViewHeight());

        auto layout = Utils::emptyHLayout(this);
        layout->setContentsMargins(Utils::scale_value(12), 0, Utils::scale_value(12), 0);
        layout->setSpacing(Utils::scale_value(8));
        layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

    void DialogSearchViewHeader::setChatNameFilter(const QString& _name)
    {
        auto filter = new SearchFilterButton(this);
        filter->setLabel(_name);

        connect(filter, &SearchFilterButton::removeClicked, this, &DialogSearchViewHeader::removeChatNameFilter);
        addFilter(qsl("dialog"), filter);
    }

    void DialogSearchViewHeader::removeChatNameFilter()
    {
        removeFilter(qsl("dialog"));

        Q_EMIT contactFilterRemoved();
    }

    void DialogSearchViewHeader::addFilter(const QString& _filterName, SearchFilterButton* _button)
    {
        removeFilter(_filterName);

        filters_[_filterName] = _button;
        Testing::setAccessibleName(_button, qsl("AS Search chat"));
        layout()->addWidget(_button);
    }

    void DialogSearchViewHeader::removeFilter(const QString& _filterName)
    {
        auto& btn = filters_[_filterName];
        if (btn)
        {
            layout()->removeWidget(btn);
            btn->deleteLater();

            btn = nullptr;
        }
    }
}
