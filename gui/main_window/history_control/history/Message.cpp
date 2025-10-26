#include "stdafx.h"
#include "Message.h"


#include "../../../controls/TextUnit.h"
#include "../../../types/message.h"
#include "../../../core_dispatcher.h"
#include "../../../gui_settings.h"
#include "../../../my_info.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../utils/gui_coll_helper.h"
#include "../../../utils/InterConnector.h"

#include "../../history_control/ChatEventInfo.h"
#include "../../history_control/FileSharingInfo.h"
#include "../../history_control/VoipEventInfo.h"
#include "../../history_control/VoipEventItem.h"
#include "../../history_control/complex_message/ComplexMessageItem.h"
#include "../../history_control/complex_message/ComplexMessageItemBuilder.h"

#include <boost/scope_exit.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include "../../mediatype.h"

namespace Logic
{
    const MessageKey MessageKey::MAX(std::numeric_limits<qint64>::max(), std::numeric_limits<qint64>::max() - 1, QString(), -1, 0, core::message_type::base, false, ControlType::Message, QDate());
    const MessageKey MessageKey::MIN(2, 1, QString(), -1, 0, core::message_type::base, false, ControlType::Message, QDate());

    MessageKey::MessageKey()
        : MessageKey(-1, ControlType::Message)
    {
    }

    MessageKey::MessageKey(const qint64 _id, const QString& _internal_id)
        : MessageKey(
            _id,
            -1,
            _internal_id,
            Data::MessageBuddy::makePendingId(_internal_id),
            -1,
            core::message_type::base,
            false,
            ControlType::Message,
            QDate())
    {

    }

    MessageKey::MessageKey(const qint64 _id, const ControlType _control_type)
        : MessageKey(
            _id,
            -1,
            QString(),
            -1,
            -1,
            core::message_type::base,
            false,
            _control_type,
            QDate())
    {
    }

    MessageKey::MessageKey(
        const qint64 _id,
        const qint64 _prev,
        const QString& _internalId,
        const qint64 _pendingId,
        const qint32 _time,
        const core::message_type _type,
        const bool _outgoing,
        const ControlType _control_type,
        const QDate _date)
        : id_(_id)
        , prev_(_prev)
        , internalId_(_internalId)
        , type_(_type)
        , controlType_(_control_type)
        , time_(_time)
        , pendingId_(_pendingId)
        , outgoing_(_outgoing)
        , date_(_date)
    {
        im_assert(type_ > core::message_type::min);
        im_assert(type_ < core::message_type::max);
    }

    bool MessageKey::operator<(const MessageKey& _other) const noexcept
    {
        return lessThan(_other);
    }

    bool MessageKey::hasId() const noexcept
    {
        return id_ != -1;
    }

    bool MessageKey::checkInvariant() const noexcept
    {
        if (outgoing_)
        {
            if (!hasId() && internalId_.isEmpty())
                return false;
        }
        else
        {
            if (!internalId_.isEmpty())
                return false;
        }

        return true;
    }

    bool MessageKey::isChatEvent() const noexcept
    {
        im_assert(type_ >= core::message_type::min);
        im_assert(type_ <= core::message_type::max);

        return type_ == core::message_type::chat_event;
    }

    bool MessageKey::isOutgoing() const noexcept
    {
        return outgoing_;
    }

    void MessageKey::setId(const int64_t _id) noexcept
    {
        im_assert(_id >= -1);

        id_ = _id;
    }

    qint64 MessageKey::getId() const noexcept
    {
        return id_;
    }

    void MessageKey::setOutgoing(const bool _isOutgoing) noexcept
    {
        outgoing_ = _isOutgoing;

        if (!hasId() && outgoing_)
        {
            im_assert(!internalId_.isEmpty());
        }
    }

    void MessageKey::setControlType(const ControlType _controlType) noexcept
    {
        controlType_ = _controlType;
    }

    QString MessageKey::toLogStringShort() const
    {
        return u"id=" % QString::number(id_) % u";prev=" % QString::number(prev_);
    }

    bool MessageKey::isFileSharing() const noexcept
    {
        im_assert(type_ >= core::message_type::min);
        im_assert(type_ <= core::message_type::max);

        return type_ == core::message_type::file_sharing;
    }

    bool MessageKey::isVoipEvent() const noexcept
    {
        im_assert(type_ >= core::message_type::min);
        im_assert(type_ <= core::message_type::max);

        return type_ == core::message_type::voip_event;
    }

    bool MessageKey::isDate() const noexcept
    {
        return controlType_ == ControlType::Date;
    }

    bool MessageKey::isNewMessagesPlate() const noexcept
    {
        return controlType_ == ControlType::NewMessages;
    }

    bool MessageKey::isSticker() const noexcept
    {
        im_assert(type_ >= core::message_type::min);
        im_assert(type_ <= core::message_type::max);

        return (type_ == core::message_type::sticker);
    }

    ControlType MessageKey::getControlType() const noexcept
    {
        return controlType_;
    }

    core::message_type MessageKey::getType() const noexcept
    {
        return type_;
    }

    void MessageKey::setType(core::message_type _type) noexcept
    {
        type_ = _type;
    }

    qint64 MessageKey::getPrev() const noexcept
    {
        return prev_;
    }

    const QString& MessageKey::getInternalId() const noexcept
    {
        return internalId_;
    }

    int MessageKey::getPendingId() const noexcept
    {
        return pendingId_;
    }

    qint32 MessageKey::getTime() const noexcept
    {
        return time_;
    }

    QDate MessageKey::getDate() const noexcept
    {
        return date_;
    }

    MessageKey MessageKey::toPurePending() const
    {
        im_assert(!isPending() && !internalId_.isEmpty());
        auto res = *this;
        res.id_ = -1;
        res.prev_ = -1;
        res.pendingId_ = Data::MessageBuddy::makePendingId(internalId_);
        return res;
    }

    bool MessageKey::lessThan(const MessageKey& _rhs) const noexcept
    {
        if (!internalId_.isEmpty() && internalId_ == _rhs.internalId_)
        {
            return controlType_ < _rhs.controlType_;
        }

        if (pendingId_ != -1 && _rhs.pendingId_ == -1)
        {
            if (controlType_ == _rhs.controlType_)
                return false;

            // without check for control_type::ct_date new message plate will be broken
            if (controlType_ != _rhs.controlType_ && _rhs.controlType_ == ControlType::Date)
                return controlType_ < _rhs.controlType_;


            return time_ == _rhs.time_ ? false : time_ < _rhs.time_;
        }

        if (pendingId_ == -1 && _rhs.pendingId_ != -1)
        {
            if (controlType_ == _rhs.controlType_)
                return true;

            // without check for control_type::ct_date new message plate will be broken
            if (controlType_ != _rhs.controlType_ && controlType_ == ControlType::Date)
                return controlType_ < _rhs.controlType_;

            return time_ == _rhs.time_ ? true : time_ < _rhs.time_;
        }

        if ((id_ != -1) && (_rhs.id_ != -1))
        {
            if (id_ != _rhs.id_)
                return id_ < _rhs.id_;
        }

        if (pendingId_ != -1 && _rhs.pendingId_ != -1)
        {
            if (internalId_ != _rhs.internalId_)
            {
                if (pendingId_ != _rhs.pendingId_)
                    return pendingId_ < _rhs.pendingId_;
                else if (time_ != _rhs.time_)
                    return time_ < _rhs.time_;
            }
        }

        return controlType_ < _rhs.controlType_;
    }


    /// Message

    bool Message::isBase() const noexcept
    {
        im_assert(key_.getType() > core::message_type::min);
        im_assert(key_.getType() < core::message_type::max);

        return key_.getType() == core::message_type::base;
    }

    bool Message::isChatEvent() const noexcept
    {
        im_assert(key_.getType() > core::message_type::min);
        im_assert(key_.getType() < core::message_type::max);

        return key_.getType() == core::message_type::chat_event;
    }

    bool Message::isDate() const noexcept
    {
        return key_.isDate();
    }

    bool Message::isNewMessagesPlate() const noexcept
    {
        return key_.isNewMessagesPlate();
    }

    bool Message::isDeleted() const noexcept
    {
        return deleted_;
    }

    void Message::setDeleted(const bool _deleted)
    {
        deleted_ = _deleted;

        if (buddy_)
            buddy_->SetDeleted(_deleted);
    }

    bool Message::isFileSharing() const noexcept
    {
        im_assert(key_.getType() > core::message_type::min);
        im_assert(key_.getType() < core::message_type::max);
        im_assert((key_.getType() != core::message_type::file_sharing) || fileSharing_);

        return key_.getType() == core::message_type::file_sharing;
    }

    bool Message::isOutgoing() const noexcept
    {
        im_assert(!buddy_ || key_.isOutgoing() == getBuddy()->IsOutgoing());
        return key_.isOutgoing();
    }

    bool Message::isPending() const noexcept
    {
        return !key_.hasId() && !key_.getInternalId().isEmpty();
    }

    bool Message::isStandalone() const noexcept
    {
        return true;
        //return (IsFileSharing() || IsSticker() || IsChatEvent() || IsVoipEvent() || IsPreview() || IsDeleted() || IsPending());
    }

    bool Message::isSticker() const noexcept
    {
        im_assert(key_.getType() > core::message_type::min);
        im_assert(key_.getType() < core::message_type::max);
        im_assert((key_.getType() != core::message_type::sticker) || sticker_);

        return key_.getType() == core::message_type::sticker;
    }

    bool Message::isVoipEvent() const noexcept
    {
        im_assert(key_.getType() > core::message_type::min);
        im_assert(key_.getType() < core::message_type::max);
        im_assert((key_.getType() != core::message_type::voip_event) || voipEvent_);

        return (key_.getType() == core::message_type::voip_event);
    }

    const HistoryControl::ChatEventInfoSptr& Message::getChatEvent() const
    {
        im_assert(!chatEvent_ || isChatEvent());

        return chatEvent_;
    }

    const QString& Message::getChatSender() const
    {
        return chatSender_;
    }

    const HistoryControl::FileSharingInfoSptr& Message::getFileSharing() const
    {
        im_assert(!fileSharing_ || isFileSharing());

        return fileSharing_;
    }


    const HistoryControl::StickerInfoSptr& Message::getSticker() const
    {
        im_assert(!sticker_ || isSticker());

        return sticker_;
    }

    const HistoryControl::VoipEventInfoSptr& Message::getVoipEvent() const
    {
        im_assert(!voipEvent_ || isVoipEvent());

        return voipEvent_;
    }

    void Message::setChatEvent(const HistoryControl::ChatEventInfoSptr& _info)
    {
        im_assert(!chatEvent_);
        im_assert(!_info || (key_.getType() == core::message_type::chat_event));

        chatEvent_ = _info;
    }

    void Message::setChatSender(const QString& _chatSender)
    {
        chatSender_ = _chatSender;
    }

    void Message::setFileSharing(const HistoryControl::FileSharingInfoSptr& _info)
    {
        im_assert(!fileSharing_);
        im_assert(!_info || (key_.getType() == core::message_type::file_sharing));

        fileSharing_ = _info;
    }

    void Message::setSticker(const HistoryControl::StickerInfoSptr& _info)
    {
        im_assert(!sticker_);
        im_assert(!_info || (key_.getType() == core::message_type::sticker));

        sticker_ = _info;
    }

    void Message::setVoipEvent(const HistoryControl::VoipEventInfoSptr& _info)
    {
        im_assert(!voipEvent_);
        im_assert(!_info || (key_.getType() == core::message_type::voip_event));

        voipEvent_ = _info;
    }

    void Message::applyModification(const Data::MessageBuddy& _modification)
    {
        im_assert(key_.getId() == _modification.Id_);

        EraseEventData();

        if (_modification.IsBase())
        {
            key_.setType(core::message_type::base);

            return;
        }

        if (_modification.IsChatEvent())
        {
            key_.setType(core::message_type::base);

            auto key = getKey();
            key.setType(core::message_type::chat_event);
            setKey(key);
            setChatEvent(_modification.GetChatEvent());
            getBuddy()->SetType(core::message_type::chat_event);

            return;
        }

        buddy_->ApplyModification(_modification);
    }

    void Message::EraseEventData()
    {
        fileSharing_.reset();
        sticker_.reset();
        chatEvent_.reset();
        voipEvent_.reset();
    }

    void Message::setBuddy(std::shared_ptr<Data::MessageBuddy> _buddy)
    {
        buddy_ = std::move(_buddy);
    }

    const std::shared_ptr<Data::MessageBuddy>& Message::getBuddy() const
    {
        return buddy_;
    }

    const QString& Message::getAimId() const
    {
        return aimId_;
    }

    void Message::setKey(const MessageKey& _key)
    {
        key_ = _key;
    }

    const MessageKey& Message::getKey() const
    {
        return key_;
    }

    const QDate& Message::getDate() const
    {
        return date_;
    }

    void Message::setDate(const QDate& _date)
    {
        date_ = _date;
    }

    void Message::setChatFriendly(const QString& _friendly)
    {
        chatFriendly_ = _friendly;
    }

    const QString& Message::getChatFriendly() const
    {
        return chatFriendly_;
    }
}
