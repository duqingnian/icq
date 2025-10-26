#pragma once

#include "../../../types/message.h"
#include "../../../types/chatheads.h"
#include "../../mediatype.h"
#include "../../../utils/utils.h"

namespace hist
{
    class MessageReader : public QObject
    {
        Q_OBJECT

        using MentionsMeVector = std::vector<Data::MessageBuddySptr>;
        using IdsSet = std::set<qint64>;
        struct LastReads
        {
            qint64 text_ = -1;
            qint64 mention_ = -1;
            std::once_flag flag_;
        };

    public:
        explicit MessageReader(const QString& _aimId, QObject* _parent = nullptr);
        ~MessageReader();

        void init();

        void setEnabled(bool _value);
        bool isEnabled() const;

        int getMentionsCount() const noexcept;
        Data::MessageBuddySptr getNextUnreadMention() const;

        void onMessageItemRead(const qint64 _messageId, const bool _visible);
        void onReadAllMentionsLess(const qint64 _messageId, const bool _visible);
        void onReadAllMentions();
        void onMessageItemReadVisible(const qint64 _messageId);

        void setDlgState(const Data::DlgState& _dlgState);
        void deleted(const Data::MessageBuddies& _messages);

    Q_SIGNALS:
        void mentionRead(qint64 _messageId, QPrivateSignal) const;

    private:
        void mentionMe(const QString& _contact, const Data::MessageBuddySptr& _mention);
        void sendLastReadMention(qint64 _messageId);
        void sendPartialLastRead(qint64 _messageId);

    private:
        const QString aimId_;
        LastReads lastReads_;
        MentionsMeVector mentions_;
        IdsSet pendingVisibleMessages_;
        bool enabled_ = true;
    };
}
