#pragma once

#include "../../namespaces.h"

#include "../../fonts.h"
#include "../../utils/utils.h"
#include "../../cache/emoji/Emoji.h"
#include "types/lastseen.h"
#include "RecentsModel.h"

#include "../../fonts.h"

namespace Ui
{
    class TextEditEx;
}

namespace Logic
{
    enum DrawIcons
    {
        NoNeedDrawIcons,
        NeedDrawIcons
    };
};

namespace Ui
{
    typedef std::unique_ptr<Ui::TextEditEx> TextEditExUptr;

    struct VisualDataBase
    {
        VisualDataBase();

        VisualDataBase(
            const QString& _aimId,
            const QPixmap& _avatar,
            const QString& _status,
            const bool _isHovered,
            const bool _isSelected,
            const QString& _contactName,
            const QString& _nick,
            const std::vector<QString>& _highlights,
            const Data::LastSeen& _lastSeen,
            const bool _isWithCheckBox,
            const bool _isChatMember,
            const bool _official,
            const bool _drawLastRead,
            const QPixmap& _lastReadAvatar,
            const QString& role,
            const int _UnreadsCounter,
            const bool _muted,
            const bool _notInCL,
            const bool _hasAttention,
            const bool _isCreator,
            const bool _isOnline
        );

        VisualDataBase& operator=(const VisualDataBase&) = default;

        QString AimId_;
        QPixmap Avatar_;
        QString ContactName_;
        QString nick_;

        std::vector<QString> highlights_;

        bool IsHovered_;
        bool IsSelected_;
        bool IsOnline_;

        Data::LastSeen LastSeen_;
        bool IsOnline() const noexcept { return LastSeen_.isOnline(); }
        bool HasLastSeen() const noexcept { return LastSeen_.isValid(); }

        bool isCheckedBox_;
        bool isChatMember_;
        bool isOfficial_;
        bool drawLastRead_;

        QPixmap lastReadAvatar_;

        const QString& GetStatus() const;
        bool HasStatus() const;
        void SetStatus(const QString& _status);

        QString role_;

        int unreadsCounter_;
        int membersCount_;

        bool isMuted_;
        bool notInCL_;
        bool hasAttention_;
        bool isCreator_;

        QDateTime canRemoveTill_;

    private:
        QString Status_;
    };

    QString FormatTime(const QDateTime &_time);

    int ItemWidth(bool _fromAlert, bool _isPictureOnlyView = false);

    struct ViewParams;
    int ItemWidth(const ViewParams& _viewParams);

    int CorrectItemWidth(int _itemWidth, int _fixedWidth);

    int ItemLength(bool _isWidth, double _koeff, int _addWidth);
    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent);

    struct ViewParams
    {
        ViewParams(int _regim, int _fixedWidth, int _leftMargin, int _rightMargin, bool _pictOnly = false)
            : regim_(_regim)
            , fixedWidth_(_fixedWidth)
            , leftMargin_(_leftMargin)
            , rightMargin_(_rightMargin)
            , pictOnly_(_pictOnly)
        {}

        ViewParams()
            : regim_(-1)
            , fixedWidth_(-1)
            , leftMargin_(0)
            , rightMargin_(0)
            , pictOnly_(false)
        {}

        int regim_;
        int fixedWidth_;
        int leftMargin_;
        int rightMargin_;
        bool pictOnly_;
        bool replaceFavorites_ = false;
    };

    class ContactListParams
    {
    public:

        //Common
        int itemHeight() const { return isCl_ ? Utils::scale_value(48) : platform::is_apple() ? Utils::scale_value(68) : Utils::scale_value(72); }
        int globalItemHeight() const { return Utils::scale_value(48); }
        int itemWidth() const { return Utils::scale_value(320); }
        int itemHorPadding() const { return isCl_ ? Utils::scale_value(12) : Utils::scale_value(8); }
        int moreRightPadding() const { return Utils::scale_value(8); }
        int itemHorPaddingRight() const { return Utils::scale_value(12); }
        int itemContentPadding() const { return isCl_ ? Utils::scale_value(12) : Utils::scale_value(8); }
        int getItemMiddleY() const
        {
            return getAvatarSize() / 2 + getAvatarY();
        };
        int serviceItemHeight() const { return Utils::scale_value(24); }
        int serviceButtonHeight() const { return Utils::scale_value(44); }
        int serviceItemIconPadding() const { return Utils::scale_value(12); }


        // dropdown
        int dropdownButtonHeight() const { return Utils::scale_value(40); }

        //Contact avatar
        int getAvatarX() const
        {
            return itemHorPadding();
        }

        int getAvatarY() const
        {
            return (itemHeight() - getAvatarSize()) / 2;
        }

        int getAvatarSize() const
        {
            return (!isCl_) ?
                Utils::scale_value(52) : Utils::scale_value(32);
        }

        //Contact name
        int getContactNameX(int _leftMargin = 0) const
        {
            return (!isCl_) ?
                (getAvatarX() + getAvatarSize() + itemContentPadding()) :
                (getAvatarX() + getAvatarSize() + Utils::scale_value(12) + _leftMargin);
        }

        int getContactNameYCenter() const
        {
            return Utils::scale_value(10);
        }

        int getContactNameYTop() const
        {
            if constexpr (platform::is_apple())
                return Utils::scale_value(2);
            return 0;
        }

        int nameYForMailStatus() const
        {
            return Utils::scale_value(20);
        }

        int contactNameFontSize() const
        {
            return Utils::scale_value(16);
        }

        Fonts::FontWeight contactNameFontWeight() const
        {
            if constexpr (platform::is_apple())
                return Fonts::FontWeight::Medium;
            else
                return Fonts::FontWeight::Normal;
        }

        Fonts::FontWeight membersNameFontWeight() const
        {
            return Fonts::FontWeight::Normal;
        }

        int contactNameHeight() const
        {
            return Utils::scale_value(22);
        }

        QString getContactNameStylesheet(const QString& _fontColor, const Fonts::FontWeight _fontWeight) const
        {
            im_assert(_fontWeight > Fonts::FontWeight::Min);
            im_assert(_fontWeight < Fonts::FontWeight::Max);

            const auto fontQss = Fonts::appFontFullQss(contactNameFontSize(), Fonts::defaultAppFontFamily(), _fontWeight);
            return ql1s("%1; color: %2; background-color: transparent;").arg(fontQss, _fontColor);
        }

        QFont getContactNameFont(const Fonts::FontWeight _fontWeight) const
        {
            return Fonts::appFont(contactNameFontSize(), _fontWeight);
        }

        Styling::ThemeColorKey getNameFontColor(bool _isSelected, bool _isMemberChecked, bool _isFavorites = false) const;

        //Message
        int messageFontSize() const
        {
            return 14;
        }

        int globalContactMessageFontSize() const
        {
            return Utils::scale_value(12);
        }

        int messageHeight() const
        {
            return  Utils::scale_value(24);
        }

        int messageX(int _leftMargin = 0) const
        {
            return  getContactNameX(_leftMargin);
        }

        int messageY() const
        {
            return  Utils::scale_value(30);
        }

        QString getRecentsMessageFontColor(const bool _isUnread) const;
        QString getMessageStylesheet(const int _fontSize, const bool _isUnread, const bool _isSelected) const;

        //Unknown contacts page
        int unknownsUnreadsY(bool _pictureOnly) const
        {
            return _pictureOnly ? Utils::scale_value(4) : Utils::scale_value(14);
        }
        int unknownsItemHeight() const { return  Utils::scale_value(40); }

        //Time
        int timeY() const
        {
            return Utils::scale_value(18);
        }

        QFont timeFont() const
        {
            return Fonts::appFontScaled(12);
        }

        Styling::ThemeColorKey timeFontColor(bool _isSelected) const;

        //Additional options
        QSize removeSize() const { return Utils::scale_value(QSize(28, 24)); }
        int role_offset() const { return  Utils::scale_value(24); }
        int role_ver_offset() const { return Utils::scale_value(4); }

        //Groups in Contact list
        int groupY()  const { return Utils::scale_value(17); }
        QFont groupFont() const { return Fonts::appFontScaled(12); }
        Styling::ThemeColorKey groupColor() const;

        //Unreads counter
        QFont unreadsFont() const
        {
            return Fonts::appFontScaled(13, platform::is_apple() ? Fonts::FontWeight::Medium : Fonts::FontWeight::SemiBold);
        }

        int unreadsPadding() const { return  Utils::scale_value(8); }

        //Last seen
        int lastReadY() const { return  Utils::scale_value(38); }
        int getLastReadAvatarSize() const { return Utils::scale_value(14); }
        int getlastReadLeftMargin() const { return Utils::scale_value(4); }
        int getlastReadRightMargin() const { return Utils::scale_value(4); }


        QFont emptyIgnoreListFont() const { return Fonts::appFontScaled(16); }

        static int dragOverlayPadding() { return Utils::scale_value(4); }
        static int dragOverlayBorderWidth() { return Utils::scale_value(1); }
        static int dragOverlayBorderRadius() { return Utils::scale_value(8); }
        static int dragOverlayVerPadding() { return  Utils::scale_value(1); }

        int official_hor_padding() const { return  Utils::scale_value(2); }
        int mute_hor_padding() const { return  Utils::scale_value(2); }

        int globalSearchHeaderHeight() const { return Utils::scale_value(32); }
        QFont getContactListHeadersFont() const { return Fonts::appFontScaled(10, Fonts::FontWeight::SemiBold); }

        ContactListParams(bool _isCl)
            : isCl_(_isCl)
        {}

        bool isCL() const
        {
            return isCl_;
        }

        void setIsCL(bool _isCl)
        {
            isCl_ = _isCl;
        }

        int pinnedStatusPadding() { return  Utils::scale_value(2); }

        QRect& addContactFrame()
        {
            static QRect addContactFrameRect(0, 0, 0, 0);
            return addContactFrameRect;
        }

        QRect& removeContactFrame()
        {
            static QRect removeContactFrameRect(0, 0, 0, 0);
            return removeContactFrameRect;
        }

    private:
        bool isCl_;
    };

    ContactListParams& GetContactListParams();
    ContactListParams& GetRecentsParams(int _regim);
    ContactListParams& GetRecentsParams();

    void RenderMouseState(QPainter &_painter, const bool isHovered, const bool _isSelected, const Ui::ViewParams& _viewParams, const int _height);
    void RenderMouseState(QPainter &_painter, const bool isHovered, const bool _isSelected, const QRect& _rect);

    void renderDragOverlay(QPainter& _painter, const QRect& _rect, const ViewParams& _viewParams);

    int GetXOfRemoveImg(int width);

    bool IsSelectMembers(int regim);

    QString getStateString(const QString& _state);
    QString getStateString(const core::profile_state _state);
    Data::FString makeDraftText(const QString& _aimId);
}

namespace Logic
{
    class AbstractItemDelegateWithRegim : public QItemDelegate
    {
        Q_OBJECT

    public:
        AbstractItemDelegateWithRegim(QObject* parent)
            : QItemDelegate(parent)
        {}

        virtual void setRegim(int _regim) = 0;

        virtual void setFixedWidth(int width) = 0;

        virtual void blockState(bool value) = 0;

        virtual void setDragIndex(const QModelIndex& index) = 0;

        int32_t avatarProxyFlags() const;
        int32_t friendlyProxyFlags() const;

        void setReplaceFavorites(bool _enable)
        {
            replaceFavorites_ = _enable;
        }

        void setKeyboardFocus(const bool _focused)
        {
            keyboardFocused_ = _focused;
        }

        bool isKeyboardFocused() const noexcept
        {
            return keyboardFocused_;
        }

        void setOpacityEnabled(bool _enable)
        {
            opacityEnabled_ = _enable;
        }

        bool isOpacityEnabled() const noexcept
        {
            return opacityEnabled_;
        }

        virtual bool needsTooltip(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const { return false; };
        virtual QRect getDraftIconRectWrapper(const QString& _aimId, const QModelIndex& _index, QPoint _posCursor = {}) const { return {}; };

    protected:
        bool keyboardFocused_ = false;
        bool replaceFavorites_ = false;
        bool opacityEnabled_ = false;
    };
}

struct QObjectDeleteLater
{
    void operator()(QObject* _object)
    {
        if (_object)
            _object->deleteLater();
    }
};

template<typename T>
using QObjectUniquePtr = std::unique_ptr<T, QObjectDeleteLater>;

template<typename T, class... Args>
QObjectUniquePtr<T> makeUniqueQObjectPtr(Args&&... args)
{
    static_assert(std::is_base_of_v<QObject, T>);
    return QObjectUniquePtr<T>(new T(std::forward<Args>(args)...));
};




