#pragma once

#include "../../types/contact.h"

namespace Logic
{
    class AvatarStorage : public QObject
    {
        friend AvatarStorage* GetAvatarStorage();

        Q_OBJECT

    Q_SIGNALS:
        void avatarChanged(const QString& aimId);

    private Q_SLOTS:
        void avatarLoaded(int64_t _seq, const QString& _aimId, QPixmap& _avatar, int _size, bool _result);

        void cleanup();

    public Q_SLOTS:
        void updateAvatar(const QString& _aimId, bool force = true);

    public:
        ~AvatarStorage();

        QPixmap Get(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate);

        QPixmap GetRounded(const QString& _aimId, const QString& _displayName, const int _sizePx, bool& _isDefault, bool _regenerate, bool mini_icons);

        QString GetLocal(const QString& _aimId, const QString& _displayName, const int _sizePx);

        void UpdateDefaultAvatarIfNeed(const QString& _aimId);
        void ForceRequest(const QString& _aimId, const int _sizePx);

        void SetAvatar(const QString& _aimId, const QPixmap& _pixmap);

        using CacheMap = std::map<QString, QPixmap>;
        const CacheMap& GetByAimIdAndSize() const;
        const CacheMap& GetByAimId() const;
        const CacheMap& GetRoundedByAimId() const;

        bool isDefaultAvatar(const QString& _aimId) const;

    private:
        AvatarStorage();

        void CleanupSecondaryCaches(QStringView _aimId, bool _isRoundedAvatarsClean = true);

        QPixmap GetRounded(const QPixmap& _avatar, const QString& _aimId, bool mini_icons, bool _isDefault);

    private Q_SLOTS:
        void onThemeChange();

    private:
        CacheMap AvatarsByAimIdAndSize_;

        CacheMap AvatarsByAimId_;

        CacheMap RoundedAvatarsByAimIdAndSize_;

        std::set<QString> RequestedAvatars_;

        QStringList LoadedAvatars_;

        QStringList LoadedAvatarsFails_;

        std::map<QString, QDateTime> TimesCache_;

        QTimer* Timer_;

        std::set<int64_t> requests_;
    };

    AvatarStorage* GetAvatarStorage();
}
