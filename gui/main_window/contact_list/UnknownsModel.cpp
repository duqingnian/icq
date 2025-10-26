#include "stdafx.h"
#include "UnknownsModel.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "../MainPage.h"
#include "../../cache/avatars/AvatarStorage.h"
#include "../../main_window/MainWindow.h"
#include "../../core_dispatcher.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"
#include "../common.shared/config/config.h"
#include "../containers/StatusContainer.h"
#include "Common.h"

namespace
{
    constexpr std::chrono::milliseconds SORT_TIMEOUT = build::is_debug() ? std::chrono::minutes(2) : std::chrono::seconds(1);
}

namespace Logic
{
    QObjectUniquePtr<UnknownsModel> g_unknownsModel;

    UnknownsModel::UnknownsModel(QObject* _parent)
        : CustomAbstractListModel(_parent)
        , timer_(new QTimer(this))
        , isHeaderVisible_(true)
    {
        connect(GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &UnknownsModel::contactChanged);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::activeDialogHide, this, &UnknownsModel::activeDialogHide);
        connect(Ui::GetDispatcher(), &Ui::core_dispatcher::dlgStates, this, &UnknownsModel::dlgStates);
        connect(Logic::GetStatusContainer(), &Logic::StatusContainer::statusChanged, this, [this](const QString& _aimid)
        {
            contactChanged(_aimid);
        });

        timer_->setInterval(SORT_TIMEOUT);
        timer_->setSingleShot(true);
        connect(timer_, &QTimer::timeout, this, &UnknownsModel::sortDialogs);

        auto remover = [=](const QString& _aimId, bool _passToRecents)
        {
            const auto wasNotEmpty = !dialogs_.empty();
            auto iter = std::find_if(
                dialogs_.begin(), dialogs_.end(), [&_aimId](const Data::DlgState& _item)
            {
                return _item.AimId_ == _aimId;
            });
            if (iter != dialogs_.end())
            {
                const bool isNowSelected = (Logic::getContactListModel()->selectedContact() == _aimId);
                if (_passToRecents)
                {
                    Logic::getRecentsModel()->unknownToRecents(*iter);

                    if (isNowSelected)
                        Utils::InterConnector::instance().openDialog(_aimId);
                }
                else if (isNowSelected)
                {
                    Utils::InterConnector::instance().closeDialog();
                }

                dialogs_.erase(iter);
                sortDialogs();
            }
            if (wasNotEmpty && dialogs_.empty())
            {
                Q_EMIT updatedSize();
                Q_EMIT Utils::InterConnector::instance().unknownsGoBack();
            }
        };

        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_added, this, [=](const QString& _aimId, bool /*succeeded*/)
        {
            remover(_aimId, true);
        });
        connect(Logic::getContactListModel(), &Logic::ContactListModel::contact_removed, this, [=](const QString& _aimId)
        {
            remover(_aimId, false);
        });

        connect(Logic::getContactListModel(), &ContactListModel::contactChanged, this, &UnknownsModel::contactChanged);
        connect(Logic::getContactListModel(), &ContactListModel::selectedContactChanged, this, &UnknownsModel::selectedContactChanged);

        connect(&Utils::InterConnector::instance(), &Utils::InterConnector::unknownsDeleteThemAll, this, [=]()
        {
            for (const auto& it : std::exchange(dialogs_, {}))
            {
                Logic::getContactListModel()->removeContactFromCL(it.AimId_);
                Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                collection.set_value_as_qstring("contact", it.AimId_);
                Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());
            }
            indexes_.clear();
            Q_EMIT updatedSize();
            Q_EMIT Utils::InterConnector::instance().unknownsGoBack();
        });
    }

    UnknownsModel::~UnknownsModel()
    {
        //
    }

    int UnknownsModel::rowCount(const QModelIndex &) const
    {
        return (int)dialogs_.size() + (isHeaderVisible_ ? 1 : 0);
    }

    int UnknownsModel::itemsCount() const
    {
        return (int)dialogs_.size();
    }

    void UnknownsModel::add(const QString& _aimId)
    {
        Data::DlgState st;
        st.AimId_ = _aimId;
        dialogs_.push_back(std::move(st));

        Logic::getContactListModel()->setContactVisible(_aimId, false);

        if (!timer_->isActive())
            timer_->start();
    }

    QVariant UnknownsModel::data(const QModelIndex& _i, int _r) const
    {
        if (!_i.isValid() || (_r != Qt::DisplayRole && !Testing::isAccessibleRole(_r)))
            return QVariant();

        int cur = _i.row();
        if (cur >= rowCount(_i))
            return QVariant();

        if (!cur && isHeaderVisible_)
        {
            static const auto st = []() {
                Data::DlgState s;
                s.AimId_ = qsl("~new_contacts~");
                s.SetText(QT_TRANSLATE_NOOP("contact_list", "NEW CONTACTS"));
                return s;
            }();

            return QVariant::fromValue(st);
        }

        cur = correctIndex(cur);

        if (cur >= (int) dialogs_.size() || cur < 0)
            return QVariant();

        const Data::DlgState& cont = dialogs_[cur];

        if (Testing::isAccessibleRole(_r))
            return QString(u"AS Unknowns " % cont.AimId_);

        return QVariant::fromValue(cont);
    }

    void UnknownsModel::contactChanged(const QString& _aimId)
    {
        const auto it = indexes_.constFind(_aimId);
        if (it != indexes_.cend() && it.value() != -1 && size_t(it.value()) < dialogs_.size())
        {
            Q_EMIT dlgStateChanged(dialogs_[it.value()]);
            const auto idx = index(it.value());
            Q_EMIT dataChanged(idx, idx);
        }
    }

    void UnknownsModel::selectedContactChanged(const QString& _new, const QString& _prev)
    {
        const auto updateIndex = [this](const QString& _aimId)
        {
            if (const auto it = indexes_.constFind(_aimId); it != indexes_.cend() && it.value() != -1)
            {
                const auto idx = index(it.value());
                Q_EMIT dataChanged(idx, idx);
            }
        };

        updateIndex(_new);
        updateIndex(_prev);
    }

    void UnknownsModel::activeDialogHide(const QString& _aimId, Ui::ClosePage _closePage)
    {
        Q_UNUSED(_closePage)
        const auto wasNotEmpty = !dialogs_.empty();
        const auto iter = std::find_if(dialogs_.begin(), dialogs_.end(), [&_aimId](const auto& _dlg) { return _dlg.AimId_ == _aimId; });
        if (iter != dialogs_.end())
        {
            QString hideContact = iter->AimId_;
            contactChanged(hideContact);
            indexes_[iter->AimId_] = -1;
            dialogs_.erase(iter);

            if (Logic::getContactListModel()->selectedContact() == _aimId)
                Utils::InterConnector::instance().closeDialog();

            Q_EMIT updatedSize();
        }
        if (wasNotEmpty && dialogs_.empty())
        {
            Q_EMIT updatedSize();
            Q_EMIT Utils::InterConnector::instance().unknownsGoBack();
            if (!Logic::getRecentsModel()->rowCount())
                Q_EMIT Utils::InterConnector::instance().showRecentsPlaceholder();
        }
    }

    void UnknownsModel::dlgStates(const QVector<Data::DlgState>& _states)
    {
        bool syncSort = false;
        auto updatedItems = 0;

        const auto curSelected = Logic::getContactListModel()->selectedContact();
        const auto w = Utils::InterConnector::instance().getMainWindow();
        const auto mainPageOpened = w && w->isUIActive() && w->isMessengerPageContactDialog();

        for (const auto& _dlgState : _states)
        {
            if (_dlgState.Chat_)
                continue;

            auto iter = std::find(dialogs_.begin(), dialogs_.end(), _dlgState);

            if (!_dlgState.isSuspicious_ || !config::get().is_on(config::features::unknown_contacts))
            {
                if (iter != dialogs_.end())
                {
                    indexes_[iter->AimId_] = -1;
                    dialogs_.erase(iter);
                    ++updatedItems;

                    Q_EMIT updatedSize();
                    Q_EMIT Utils::InterConnector::instance().unknownsGoBack();
                }

                continue;
            }

            if (iter != dialogs_.end())
            {
                ++updatedItems;
                auto &existingDlgState = *iter;

                if (existingDlgState.YoursLastRead_ != _dlgState.YoursLastRead_)
                    Q_EMIT readStateChanged(_dlgState.AimId_);

                const auto existingText = existingDlgState.GetText();

                existingDlgState = _dlgState;

                const auto mustRecoverText = (!existingDlgState.HasText() && existingDlgState.HasLastMsgId());
                if (mustRecoverText)
                {
                    existingDlgState.SetText(existingText);
                }

                if (!syncSort)
                {
                    if (!timer_->isActive())
                        timer_->start();

                    const auto idx = index((int)std::distance(dialogs_.begin(), iter));
                    Q_EMIT dataChanged(idx, idx);
                }
            }
            else if (!_dlgState.GetText().isEmpty())
            {
                ++updatedItems;
                dialogs_.push_back(_dlgState);

                if (indexes_.empty())
                    syncSort = true;

                Logic::getContactListModel()->setContactVisible(_dlgState.AimId_, false);

                if (!timer_->isActive())
                    timer_->start();

                if (dialogs_.size() == 1)
                {
                    Q_EMIT updatedSize();
                    Logic::updatePlaceholders();
                }

                if (_dlgState.AimId_ == curSelected && _dlgState.Outgoing_)
                    Q_EMIT Utils::InterConnector::instance().unknownsGoSeeThem();
            }

            if (mainPageOpened && _dlgState.AimId_ == curSelected)
                sendLastRead(_dlgState.AimId_);

            Q_EMIT dlgStateChanged(_dlgState);
        }

        if (syncSort)
            sortDialogs();

        if (updatedItems > 0)
            Q_EMIT updatedMessages();

        Q_EMIT dlgStatesHandled(_states);
    }

    void UnknownsModel::sortDialogs()
    {
        std::stable_sort(dialogs_.begin(), dialogs_.end(), [](const Data::DlgState& first, const Data::DlgState& second)
        {
            if (first.PinnedTime_ == -1 && second.PinnedTime_ == -1)
                return first.serverTime_ > second.serverTime_;

            if (first.PinnedTime_ == -1)
                return false;
            else if (second.PinnedTime_ == -1)
                return true;

            if (first.PinnedTime_ == second.PinnedTime_)
                return first.AimId_ > second.AimId_;

            return first.PinnedTime_ < second.PinnedTime_;
        });
        indexes_.clear();
        int i = 0;
        for (const auto& iter : dialogs_)
            indexes_[iter.AimId_] = i++;

        Q_EMIT dataChanged(index(0), index(rowCount()));
        Q_EMIT orderChanged();
    }

    void UnknownsModel::refresh()
    {
        Q_EMIT refreshAll();
        Q_EMIT dataChanged(index(0), index(rowCount()));
    }

    Data::DlgState UnknownsModel::getDlgState(const QString& _aimId, bool _fromDialog)
    {
        Data::DlgState state;
        const auto iter = std::find_if(dialogs_.begin(), dialogs_.end(), [&_aimId](const auto& _dlg) { return _dlg.AimId_ == _aimId; });
        if (iter != dialogs_.end())
            state = *iter;

        if (_fromDialog)
            sendLastRead(_aimId);

        return state;
    }

    void UnknownsModel::sendLastRead(const QString& _aimId)
    {
        const auto aimId = _aimId.isEmpty() ? Logic::getContactListModel()->selectedContact() : _aimId;
        const auto iter = std::find_if(dialogs_.begin(), dialogs_.end(), [&aimId](const auto& _dlg) { return _dlg.AimId_ == aimId; });

        if (iter != dialogs_.end())
            sendLastReadInternal(std::distance(dialogs_.begin(), iter));
    }

    void UnknownsModel::markAllRead()
    {
        for (unsigned int i = 0; i < dialogs_.size(); ++i)
            sendLastReadInternal(i);
    }

    void UnknownsModel::hideChat(const QString& _aimId)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("contact", _aimId);
        Ui::GetDispatcher()->post_message_to_core("dialogs/hide", collection.get());
    }

    bool UnknownsModel::isServiceItem(const QModelIndex& _index) const
    {
        return isHeaderVisible_ && _index.row() == 0;
    }

    bool UnknownsModel::isClickableItem(const QModelIndex& _index) const
    {
        return !isServiceItem(_index);
    }

    QModelIndex UnknownsModel::contactIndex(const QString& _aimId) const
    {
        int i = isHeaderVisible_ ? 1 : 0;
        for (const auto& iter : dialogs_)
        {
            if (iter.AimId_ == _aimId)
                return index(i);
            ++i;
        }
        return QModelIndex();
    }

    bool UnknownsModel::contains(const QString& _aimId) const
    {
        return std::any_of(dialogs_.begin(), dialogs_.end(), [&_aimId](const auto& _state) { return _state.AimId_ == _aimId; });
    }

    QString UnknownsModel::firstContact() const
    {
        if (!dialogs_.empty())
            return dialogs_.front().AimId_;
        return QString();
    }

    int UnknownsModel::unreads(size_t _index) const
    {
        auto i = correctIndex((int)_index);
        if (i < (int)dialogs_.size())
            return (int)dialogs_[i].UnreadCount_;
        return 0;
    }

    int UnknownsModel::totalUnreads() const
    {
        int result = 0;
        for (const auto& iter : dialogs_)
        {
            if (!Logic::getContactListModel()->isMuted(iter.AimId_))
                result += iter.UnreadCount_;
        }
        return result;
    }

    QString UnknownsModel::nextUnreadAimId() const
    {
        for (const auto& iter : dialogs_)
        {
            if (iter.Attention_ || (iter.UnreadCount_ > 0 && !Logic::getContactListModel()->isMuted(iter.AimId_)))
                return iter.AimId_;
        }

        return QString();
    }

    bool UnknownsModel::hasAttentionDialogs() const
    {
        return std::any_of(dialogs_.begin(), dialogs_.end(), [](const auto& x) { return x.Attention_; });
    }

    QString UnknownsModel::nextAimId(const QString& _aimId) const
    {
        for (size_t i = 0; i < dialogs_.size() - 1; ++i)
        {
            if (dialogs_[i].AimId_ == _aimId)
                return dialogs_[i + 1].AimId_;
        }

        return QString();
    }

    QString UnknownsModel::prevAimId(const QString& _aimId) const
    {
        for (size_t i = 1; i < dialogs_.size(); ++i)
        {
            if (dialogs_[i].AimId_ == _aimId)
                return dialogs_[i - 1].AimId_;
        }

        return QString();
    }

    int UnknownsModel::correctIndex(int _i) const
    {
        if (isHeaderVisible_)
        {
            if (_i == 0)
                return _i;

            return _i - 1;
        }
        else
            return _i;
    }

    void UnknownsModel::sendLastReadInternal(const unsigned int _index)
    {
        if (_index >= dialogs_.size())
            return;

        auto& dlg = dialogs_[_index];
        if (dlg.UnreadCount_ == 0 || dlg.YoursLastRead_ >= dlg.LastMsgId_)
            return;

        dlg.UnreadCount_ = 0;

        Ui::GetDispatcher()->setLastRead(dlg.AimId_, dlg.LastMsgId_);

        const auto idx = index(_index);
        Q_EMIT dataChanged(idx, idx);
        Q_EMIT updatedMessages();
    }

    void UnknownsModel::setHeaderVisible(bool _isVisible)
    {
        isHeaderVisible_ = _isVisible;
    }

    UnknownsModel* getUnknownsModel()
    {
        if (!g_unknownsModel)
            g_unknownsModel = makeUniqueQObjectPtr<UnknownsModel>();

        return g_unknownsModel.get();
    }

    void ResetUnknownsModel()
    {
        g_unknownsModel.reset();
    }
}
