#include "stdafx.h"
#include "chat.h"

#include "../../corelib/collection_helper.h"

#include "my_info.h"
#include "utils/features.h"

namespace Data
{
    QVector<ChatMemberInfo> UnserializeChatMembers(core::coll_helper* helper, const QString& _creator)
    {
        core::iarray* membersArray;
        if (helper->is_value_exist("members"))
            membersArray = helper->get_value_as_array("members");
        else
            membersArray = helper->get_value_as_array("subscribers");

        const auto size = membersArray->size();
        QVector<ChatMemberInfo> members;
        members.reserve(size);

        for (core::iarray::size_type i = 0; i < size; ++i)
        {
            ChatMemberInfo member;
            core::coll_helper value(membersArray->get_at(i)->get_as_collection(), false);
            member.AimId_ = QString::fromUtf8(value.get_value_as_string("aimid"));
            member.Role_ = QString::fromUtf8(value.get_value_as_string("role", ""));
            member.Lastseen_ = LastSeen(value);
            member.IsCreator_ = _creator.isEmpty() ? member.Role_ == u"admin" : member.AimId_ == _creator;
            if (value.is_value_exist("canRemoveTill") && member.Role_ != u"admin" && member.Role_ != u"moder")
                member.canRemoveTill_ = QDateTime::fromSecsSinceEpoch(value.get_value_as_int64("canRemoveTill"));

            members.push_back(std::move(member));
        }
        return members;
    }

    ChatInfo UnserializeChatInfo(core::coll_helper* helper)
    {
        ChatInfo info;
        info.AimId_ = QString::fromUtf8(helper->get_value_as_string("aimid"));
        info.Name_ = QString::fromUtf8(helper->get_value_as_string("name")).trimmed();
        info.Location_ = QString::fromUtf8(helper->get_value_as_string("location"));
        info.Stamp_ = QString::fromUtf8(helper->get_value_as_string("stamp"));
        info.About_ = QString::fromUtf8(helper->get_value_as_string("about"));
        info.Rules_ = QString::fromUtf8(helper->get_value_as_string("rules"));
        info.YourRole_ = QString::fromUtf8(helper->get_value_as_string("your_role"));
        info.Owner_ = QString::fromUtf8(helper->get_value_as_string("owner"));
        info.Creator_ = QString::fromUtf8(helper->get_value_as_string("creator"));
        info.DefaultRole_ = QString::fromUtf8(helper->get_value_as_string("default_role"));
        info.Inviter_ = QString::fromUtf8(helper->get_value_as_string("inviter"));
        info.MembersVersion_ = QString::fromUtf8(helper->get_value_as_string("members_version"));
        info.InfoVersion_ = QString::fromUtf8(helper->get_value_as_string("info_version"));
        info.CreateTime_ =  helper->get_value_as_int("create_time");
        info.AdminsCount_ = helper->get_value_as_int("admins_count");
        info.MembersCount_ =  helper->get_value_as_int("members_count");
        info.FriendsCount =  helper->get_value_as_int("friend_count");
        info.BlockedCount_ =  helper->get_value_as_int("blocked_count");
        info.PendingCount_ =  helper->get_value_as_int("pending_count");
        info.YourInvitesCount_ =  helper->get_value_as_int("your_invites_count");
        info.YouBlocked_ = helper->get_value_as_bool("you_blocked");
        info.YouPending_ = helper->get_value_as_bool("you_pending");
        info.YouMember_ = helper->get_value_as_bool("you_member");
        info.Public_ = helper->get_value_as_bool("public");
        info.Live_ = helper->get_value_as_bool("live");
        info.Controlled_ = helper->get_value_as_bool("controlled");
        info.ApprovedJoin_ = helper->get_value_as_bool("joinModeration");
        info.trustRequired_ = helper->get_value_as_bool("trustRequired") && Features::isRestrictedFilesEnabled();
        info.ThreadsEnabed_ = helper->get_value_as_bool("threadsEnabled");
        info.threadsAutoSubscribe_ = helper->get_value_as_bool("threadsAutoSubscribe") && Features::isThreadsEnabled();
        info.Members_ = UnserializeChatMembers(helper, info.Creator_);
        return info;
    }

    ChatMembersPage UnserializeChatMembersPage(core::coll_helper* _helper)
    {
        ChatMembersPage info;
        info.AimId_ = QString::fromUtf8(_helper->get_value_as_string("aimid"));
        if (_helper->is_value_exist("cursor"))
            info.Cursor_ = QString::fromUtf8(_helper->get_value_as_string("cursor"));
        info.Members_ = UnserializeChatMembers(_helper);
        return info;
    }

    ChatContacts UnserializeChatContacts(core::coll_helper* _helper)
    {
        ChatContacts info;
        info.AimId_ = QString::fromUtf8(_helper->get_value_as_string("aimid"));
        info.Cursor_ = QString::fromUtf8(_helper->get_value_as_string("cursor"));

        core::iarray* membersArray = _helper->get_value_as_array("members");
        const auto size = membersArray->size();
        QVector<QString> members;
        members.reserve(size);
        for (core::iarray::size_type i = 0; i < size; ++i)
        {
            core::coll_helper value(membersArray->get_at(i)->get_as_collection(), false);
            members.append(QString::fromUtf8(value.get_value_as_string("aimid")));
        }
        info.Members_ = std::move(members);
        return info;
    }

    ChatResult UnserializeChatHome(core::coll_helper* helper)
    {
        ChatResult result;
        result.newTag = QString::fromUtf8(helper->get_value_as_string("new_tag"));
        result.restart = helper->get_value_as_bool("need_restart");
        result.finished = helper->get_value_as_bool("finished");
        core::iarray* chatsArray = helper->get_value_as_array("chats");
        const auto size = chatsArray->size();
        QVector<ChatInfo> chats;
        chats.reserve(size);
        for (core::iarray::size_type i = 0; i < chatsArray->size(); ++i)
        {
            core::coll_helper value(chatsArray->get_at(i)->get_as_collection(), false);
            chats.append(UnserializeChatInfo(&value));
        }
        result.chats = std::move(chats);
        return result;
    }

    DialogGalleryEntry UnserializeGalleryEntry(core::coll_helper* _helper)
    {
        DialogGalleryEntry entry;
        entry.msg_id_ = _helper->get_value_as_int64("msg_id");
        entry.seq_ = _helper->get_value_as_int64("seq");
        entry.url_ = QString::fromUtf8(_helper->get_value_as_string("url"));
        entry.type_ = QString::fromUtf8(_helper->get_value_as_string("type"));
        entry.sender_ = QString::fromUtf8(_helper->get_value_as_string("sender"));
        entry.outgoing_ = _helper->get_value_as_bool("outgoing");
        entry.time_ = _helper->get_value_as_int("time");
        entry.action_ = QString::fromUtf8(_helper->get_value_as_string("action"));
        entry.caption_ = QString::fromUtf8(_helper->get_value_as_string("caption"));

        return entry;
    }

    DialogGalleryState UnserializeDialogGalleryState(core::coll_helper* _helper)
    {
        DialogGalleryState state;
        state.imagesCount_ = _helper->get_value_as_int("images");
        state.videosCount_ = _helper->get_value_as_int("videos");
        state.filesCount_ = _helper->get_value_as_int("files");
        state.linksCount_ = _helper->get_value_as_int("links");
        state.pttCount_ = _helper->get_value_as_int("ptt");
        state.audioCount_ = _helper->get_value_as_int("audio");

        return state;
    }

    bool ChatInfo::areYouMember(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info)
            return false;

        return _info->YouMember_ || (!_info->YourRole_.isEmpty() && _info->YourRole_ != u"notamember" && _info->YourRole_ != u"pending");
    }

    bool ChatInfo::areYouAdmin(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        if (!_info)
            return false;

        return _info->YourRole_ == u"admin" || _info->Creator_ == Ui::MyInfo()->aimId();
    }

    bool ChatInfo::areYouModer(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        return _info && _info->YourRole_ == u"moder";
    }

    bool ChatInfo::areYouAdminOrModer(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        return areYouAdmin(_info) || areYouModer(_info);
    }

    bool ChatInfo::isChatControlled(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        return _info && _info->Controlled_;
    }

    bool ChatInfo::isChannel(const std::shared_ptr<Data::ChatInfo>& _info)
    {
        return _info && _info->isChannel();
    }
}
