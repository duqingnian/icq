#pragma once

#include "message.h"
#include "chat_member.h"

class QPixmap;

namespace core
{
    class coll_helper;
}

namespace Data
{
    class ChatInfo
    {
    public:
        QString AimId_;
        QString Name_;
        QString Location_;
        QString About_;
        QString Rules_;
        QString YourRole_;
        QString Owner_;
        QString MembersVersion_;
        QString InfoVersion_;
        QString Stamp_;
        QString Creator_;
        QString DefaultRole_;
        QString Inviter_;

        int32_t CreateTime_ = -1;
        int32_t AdminsCount_ = -1;
        int32_t MembersCount_ = - 1;
        int32_t FriendsCount = -1;
        int32_t BlockedCount_ = -1;
        int32_t PendingCount_ = -1;
        int32_t YourInvitesCount_ = -1;

        bool YouBlocked_ = false;
        bool YouPending_ = false;
        bool Public_ = false;
        bool ApprovedJoin_ = false;
        bool ThreadsEnabed_ = false;
        bool Live_ = false;
        bool Controlled_ = false;
        bool YouMember_ = false;
        bool trustRequired_ = false;
        bool threadsAutoSubscribe_ = false;

        QVector<ChatMemberInfo> Members_;

        bool isChannel() const { return DefaultRole_ == u"readonly"; }

        static bool areYouMember(const std::shared_ptr<Data::ChatInfo>& _info);
        static bool areYouAdmin(const std::shared_ptr<Data::ChatInfo>& _info);
        static bool areYouModer(const std::shared_ptr<Data::ChatInfo>& _info);
        static bool areYouAdminOrModer(const std::shared_ptr<Data::ChatInfo>& _info);
        static bool isChatControlled(const std::shared_ptr<Data::ChatInfo>& _info);
        static bool isChannel(const std::shared_ptr<Data::ChatInfo>& _info);
    };

    QVector<ChatMemberInfo> UnserializeChatMembers(core::coll_helper* helper, const QString& _creator = QString());
    ChatInfo UnserializeChatInfo(core::coll_helper* helper);

    class ChatMembersPage
    {
    public:
        QString AimId_;
        QString Cursor_;

        QVector<ChatMemberInfo> Members_;
    };

    ChatMembersPage UnserializeChatMembersPage(core::coll_helper* _helper);

    class ChatContacts
    {
    public:
        QString AimId_;
        QString Cursor_;

        QVector<QString> Members_;
    };

    ChatContacts UnserializeChatContacts(core::coll_helper* _helper);

    struct ChatResult
    {
        QVector<ChatInfo> chats;
        QString newTag;
        bool restart = false;
        bool finished = false;
    };

    ChatResult UnserializeChatHome(core::coll_helper* helper);

    struct DialogGalleryEntry
    {
        qint64 msg_id_ = -1;
        qint64 seq_ = -1;
        QString url_;
        QString type_;
        QString sender_;
        bool outgoing_ = false;
        time_t time_ = 0;
        QString action_;
        QString caption_;
    };

    DialogGalleryEntry UnserializeGalleryEntry(core::coll_helper* _helper);

    struct DialogGalleryState
    {
        bool isEmpty() const { return imagesCount_ == 0 && videosCount_ == 0 && filesCount_ == 0 && linksCount_ == 0 && pttCount_ == 0; }

        int imagesCount_ = 0;
        int videosCount_ = 0;
        int filesCount_ = 0;
        int linksCount_ = 0;
        int pttCount_ = 0;
        int audioCount_ = 0;
    };

    DialogGalleryState UnserializeDialogGalleryState(core::coll_helper* _helper);

    struct CommonChatInfo
    {
        bool operator==(const CommonChatInfo& other) const
        {
            return aimid_ == other.aimid_;
        }

        QString aimid_;
        QString friendly_;
        int32_t membersCount_ = 0;
    };
}

Q_DECLARE_METATYPE(Data::ChatInfo);
Q_DECLARE_METATYPE(QVector<Data::ChatInfo>);
Q_DECLARE_METATYPE(QVector<Data::ChatMemberInfo>);
Q_DECLARE_METATYPE(Data::DialogGalleryEntry);
Q_DECLARE_METATYPE(QVector<Data::DialogGalleryEntry>);
Q_DECLARE_METATYPE(Data::DialogGalleryState);
Q_DECLARE_METATYPE(Data::CommonChatInfo);
