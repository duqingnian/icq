#include "stdafx.h"
#include "ContactItem.h"
#include "contact_profile.h"

namespace
{
    const int maxDaysForActive = 7;
}

namespace Logic
{
    ContactItem::ContactItem(Data::ContactPtr _contact)
        : contact_(std::move(_contact))
        , visible_(true)
    {
    }

    bool ContactItem::operator== (const ContactItem& _other) const
    {
        return get_aimid() == _other.get_aimid();
    }

    bool ContactItem::is_visible() const
    {
        return visible_;
    }

    void ContactItem::set_visible(bool _visible)
    {
        visible_ = _visible;
    }

    bool ContactItem::is_public() const
    {
        return contact_->IsPublic_;
    }

    Data::Contact* ContactItem::Get() const
    {
        return contact_.get();
    }

    bool ContactItem::is_group() const
    {
        return contact_->GetType() == Data::GROUP;
    }

    bool ContactItem::is_online() const
    {
        return contact_->LastSeen_.isOnline();
    }

    bool ContactItem::is_phone() const
    {
        return contact_->UserType_ == u"sms" || contact_->UserType_ == u"phone";
    }

    bool ContactItem::recently() const
    {
        return recently(QDateTime::currentDateTime());
    }

    bool ContactItem::recently(const QDateTime& _current) const
    {
        return contact_->LastSeen_.isValid() && contact_->LastSeen_.isActive() && contact_->LastSeen_.toDateTime().daysTo(_current) <= maxDaysForActive;
    }

    bool ContactItem::is_active(const QDateTime& _current) const
    {
        return (is_online() || is_chat() || is_live_chat() || recently(_current)) && !is_group();
    }

    bool ContactItem::is_chat() const
    {
        return contact_->Is_chat_;
    }

    bool ContactItem::is_muted() const
    {
        return contact_->Muted_;
    }

    bool ContactItem::is_live_chat() const
    {
        return contact_->IsLiveChat_;
    }
    bool ContactItem::is_official() const
    {
        return contact_->IsOfficial_;
    }

    bool ContactItem::is_checked() const
    {
        return contact_->IsChecked_;
    }

    void ContactItem::set_checked(bool _isChecked)
    {
        contact_->IsChecked_ = _isChecked;
    }

    void ContactItem::set_chat_role(const QString& role)
    {
        chat_role_ = role;
    }

    const QString& ContactItem::get_chat_role() const
    {
        return chat_role_;
    }

    void ContactItem::set_default_role(const QString &role)
    {
        default_role_ = role;
    }

    const QString& ContactItem::get_default_role() const
    {
        return default_role_;
    }

    const QString& ContactItem::get_aimid() const
    {
        return contact_->AimId_;
    }

    int ContactItem::get_outgoing_msg_count() const
    {
        return contact_->OutgoingMsgCount_;
    }

    void ContactItem::set_outgoing_msg_count(const int _count)
    {
        contact_->OutgoingMsgCount_ = _count;
    }

    void ContactItem::set_stamp(const QString & _stamp)
    {
        stamp_ = _stamp;
    }

    const QString& ContactItem::get_stamp() const
    {
        return stamp_;
    }

    bool ContactItem::is_channel() const
    {
        return contact_->isChannel_;
    }

    bool ContactItem::is_readonly() const
    {
        const auto chatRole = get_chat_role();
        return (chatRole.isEmpty() && is_channel()) || chatRole == u"readonly" || chatRole == u"notamember" || chatRole == u"pending";
    }

    bool ContactItem::is_deleted() const
    {
        return contact_->isDeleted_;
    }
}
