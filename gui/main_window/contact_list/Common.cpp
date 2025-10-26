#include "stdafx.h"

#include "../../main_window/MainWindow.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

#include "Common.h"
#include "RecentsTab.h"
#include "../../gui_settings.h"
#include "../proxy/AvatarStorageProxy.h"
#include "../proxy/FriendlyContaInerProxy.h"
#include "FavoritesUtils.h"

#include "styles/ThemeParameters.h"
#include "../../../common.shared/config/config.h"

namespace
{
    int getDndOverlaySize() noexcept { return Utils::scale_value(60); }
} // namespace

namespace Ui
{
    const int MAX_TOOLTIP_TEXT_LEN = 52;

    VisualDataBase::VisualDataBase()
        : IsHovered_(false)
        , IsSelected_(false)
        , IsOnline_(false)
        , isCheckedBox_(false)
        , isChatMember_(false)
        , isOfficial_(false)
        , drawLastRead_(false)
        , unreadsCounter_(0)
        , membersCount_(0)
        , isMuted_(false)
        , notInCL_(false)
        , hasAttention_(false)
        , isCreator_(false)
    {
    }

    VisualDataBase::VisualDataBase(
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
        const bool _isOfficial,
        const bool _drawLastRead,
        const QPixmap& _lastReadAvatar,
        const QString& _role,
        const int _unreadsCounter,
        const bool _muted,
        const bool _notInCL,
        const bool _hasAttention,
        const bool _isCreator,
        const bool _isOnline)
        : AimId_(_aimId)
        , Avatar_(_avatar)
        , ContactName_(_contactName)
        , nick_(_nick)
        , highlights_(_highlights)
        , IsHovered_(_isHovered)
        , IsSelected_(_isSelected)
        , IsOnline_(_isOnline)
        , LastSeen_(_lastSeen)
        , isCheckedBox_(_isWithCheckBox)
        , isChatMember_(_isChatMember)
        , isOfficial_(_isOfficial)
        , drawLastRead_(_drawLastRead)
        , lastReadAvatar_(_lastReadAvatar)
        , role_(_role)
        , unreadsCounter_(_unreadsCounter)
        , membersCount_(0)
        , isMuted_(_muted)
        , notInCL_(_notInCL)
        , hasAttention_(_hasAttention)
        , isCreator_(_isCreator)
        , Status_(_status)
    {
        im_assert(!AimId_.isEmpty());
        im_assert(!ContactName_.isEmpty());
    }

    const QString& VisualDataBase::GetStatus() const
    {
        return Status_;
    }

    bool VisualDataBase::HasStatus() const
    {
        return !Status_.isEmpty();
    }

    void VisualDataBase::SetStatus(const QString& _status)
    {
        Status_ = _status;
    }

    QString FormatTime(const QDateTime& _time)
    {
        if (!_time.isValid())
        {
            return QString();
        }

        const auto current = QDateTime::currentDateTime();

        const auto days = _time.daysTo(current);

        if (days == 0)
        {
            return _time.time().toString(Qt::SystemLocaleShortDate);
        }

        if (days == 1)
        {
            return QT_TRANSLATE_NOOP("contact_list", "yesterday");
        }

        const auto date = _time.date();
        return Utils::GetTranslator()->formatDate(date, date.year() == current.date().year());
    }

    int ItemWidth(bool _fromAlert, bool _isPictureOnlyView)
    {
        if (_isPictureOnlyView)
        {
            auto params = ::Ui::GetRecentsParams(Logic::MembersWidgetRegim::CONTACT_LIST);
            params.setIsCL(false);
            return params.itemHorPadding() + params.getAvatarSize() + params.itemHorPadding();
        }

        if (_fromAlert)
            return Ui::GetRecentsParams(Logic::MembersWidgetRegim::FROM_ALERT).itemWidth();

        //return std::min(Utils::scale_value(400), ItemLength(true, 1. / 3, 0));
        return ItemLength(true, 1. / 3, 0);
    }

    int ItemWidth(const ViewParams& _viewParams)
    {
        return ItemWidth(_viewParams.regim_ == ::Logic::MembersWidgetRegim::FROM_ALERT, _viewParams.pictOnly_);
    }

    int CorrectItemWidth(int _itemWidth, int _fixedWidth)
    {
        return _fixedWidth == -1 ? _itemWidth : _fixedWidth;
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth)
    {
        return ItemLength(_isWidth, _koeff, _addWidth, Utils::InterConnector::instance().getMainWindow());
    }

    int ItemLength(bool _isWidth, double _koeff, int _addWidth, QWidget* parent)
    {
        im_assert(!!parent && "Common.cpp (ItemLength)");
        auto mainRect = Utils::GetWindowRect(parent);
        if (mainRect.width() && mainRect.height())
        {
            auto mainLength = _isWidth ? mainRect.width() : mainRect.height();
            return _addWidth + mainLength * _koeff;
        }
        im_assert("Couldn't get rect: Common.cpp (ItemLength)");
        return 0;
    }

    Styling::ThemeColorKey ContactListParams::getNameFontColor(bool _isSelected, bool _isMemberChecked, bool _isFavorites) const
    {
        if (_isSelected)
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID_PERMANENT };
        if (_isMemberChecked)
            return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
        if (_isFavorites)
            return Favorites::nameColor();
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_SOLID };
    }

    QString ContactListParams::getRecentsMessageFontColor(const bool _isUnread) const
    {
        const auto color = _isUnread ?
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_PRIMARY) :
            Styling::getParameters().getColorHex(Styling::StyleVariable::BASE_SECONDARY);

        return color;
    };

    QString ContactListParams::getMessageStylesheet(const int _fontSize, const bool _isUnread, const bool _isSelected) const
    {
        const auto fontWeight = Fonts::FontWeight::Normal;
        const auto fontQss = Fonts::appFontFullQss(_fontSize, Fonts::defaultAppFontFamily(), fontWeight);
        const auto fontColor = _isSelected ? Styling::getParameters().getColorHex(Styling::StyleVariable::TEXT_SOLID_PERMANENT) : getRecentsMessageFontColor(_isUnread);
        return ql1s("%1; color: %2; background-color: transparent").arg(fontQss, fontColor);
    };

    Styling::ThemeColorKey ContactListParams::timeFontColor(bool _isSelected) const
    {
        return Styling::ThemeColorKey{ _isSelected ? Styling::StyleVariable::TEXT_SOLID_PERMANENT : Styling::StyleVariable::BASE_SECONDARY };
    }

    Styling::ThemeColorKey ContactListParams::groupColor() const
    {
        return Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY };
    }

    ContactListParams& GetContactListParams()
    {
        static ContactListParams params(true);
        return params;
    }

    ContactListParams& GetRecentsParams()
    {
        static ContactListParams params(false);

        params.setIsCL(false);

        return params;
    }

    ContactListParams& GetRecentsParams(int _regim)
    {
        if (_regim == Logic::MembersWidgetRegim::FROM_ALERT || _regim == Logic::MembersWidgetRegim::HISTORY_SEARCH)
        {
            static ContactListParams params(false);
            return params;
        }
        else if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            return GetContactListParams();
        }
        else
        {
            const auto show_last_message = Ui::get_gui_settings()->get_value<bool>(settings_show_last_message, !config::get().is_on(config::features::compact_mode_by_default));
            static ContactListParams params(!show_last_message);
            params.setIsCL(!show_last_message);
            return params;
        }
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const ViewParams& _viewParams, const int _height)
    {
        auto width = CorrectItemWidth(ItemWidth(_viewParams), _viewParams.fixedWidth_);
        QRect rect(0, 0, width, _height);

        RenderMouseState(_painter, _isHovered, _isSelected, rect);
    }

    void RenderMouseState(QPainter &_painter, const bool _isHovered, const bool _isSelected, const QRect& _rect)
    {
        QColor color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE);
        if (_isSelected)
            color = Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY_SELECTED);
        else if (_isHovered)
            color = Styling::getParameters().getColor(Styling::StyleVariable::BASE_BRIGHT_INVERSE);

        _painter.fillRect(_rect, color);
    }

    void renderDragOverlay(QPainter& _painter, const QRect& _rect, const ViewParams& _viewParams)
    {
        Utils::PainterSaver ps(_painter);

        _painter.setPen(Qt::NoPen);
        _painter.setRenderHint(QPainter::Antialiasing);

        const QColor overlayColor(Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE, 0.98));
        _painter.fillRect(_rect, QBrush(overlayColor));
        _painter.setBrush(QBrush(Styling::getParameters().getColor(Styling::StyleVariable::GHOST_ACCENT)));

        const QPen pen(Styling::getParameters().getColor(Styling::StyleVariable::PRIMARY), ContactListParams::dragOverlayBorderWidth(), Qt::SolidLine, Qt::RoundCap);
        _painter.setPen(pen);

        const QRect overlayRect = _rect.adjusted(
            ContactListParams::dragOverlayPadding(),
            ContactListParams::dragOverlayVerPadding(),
            -ContactListParams::dragOverlayPadding(),
            -ContactListParams::dragOverlayVerPadding());
        _painter.drawRoundedRect(overlayRect, ContactListParams::dragOverlayBorderRadius(), ContactListParams::dragOverlayBorderRadius());

        if (_viewParams.pictOnly_)
        {
            const auto overlaySize = getDndOverlaySize();
            static auto fastSendGreen = Utils::StyledPixmap(qsl(":/fast_send"), QSize{ overlaySize, overlaySize }, Styling::ThemeColorKey{ Styling::StyleVariable::TEXT_PRIMARY });
            _painter.drawPixmap(_rect.left() + _rect.width() / 2 - overlaySize / 2, _rect.top() + _rect.height() / 2 - overlaySize / 2, fastSendGreen.actualPixmap());
        }
        else
        {
            _painter.setFont(Fonts::appFontScaled(15));
            Utils::drawText(_painter, QPoint(_rect.width() / 2, _rect.y() + _rect.height() / 2), Qt::AlignCenter, QT_TRANSLATE_NOOP("files_widget", "Quick send"));
        }
    }

    int GetXOfRemoveImg(int _width)
    {
        auto _contactList = GetContactListParams();
        return
            CorrectItemWidth(ItemWidth(false, false), _width)
            - _contactList.itemHorPadding()
            - _contactList.removeSize().width()
            + Utils::scale_value(8);
    }

    bool IsSelectMembers(int _regim)
    {
        constexpr Logic::MembersWidgetRegim regimes[] =
        {
            Logic::MembersWidgetRegim::SELECT_MEMBERS,
            Logic::MembersWidgetRegim::VIDEO_CONFERENCE,
            Logic::MembersWidgetRegim::SELECT_CHAT_MEMBERS,
            Logic::MembersWidgetRegim::DISALLOWED_INVITERS_ADD,
        };
        return std::any_of(std::begin(regimes), std::end(regimes), [_regim](auto r){ return r == _regim; });
    }

    QString getStateString(const QString& _state)
    {
        if (_state == u"online")
            return getStateString(core::profile_state::online);
        else if (_state == u"offline")
            return getStateString(core::profile_state::offline);
        else if (_state == u"away")
            return getStateString(core::profile_state::away);
        else if (_state == u"dnd")
            return getStateString(core::profile_state::dnd);
        else if (_state == u"invisible")
            return getStateString(core::profile_state::invisible);

        return _state;
    }

    QString getStateString(const core::profile_state _state)
    {
        switch (_state)
        {
        case core::profile_state::online:
            return QT_TRANSLATE_NOOP("state", "Online");
        case core::profile_state::dnd:
            return QT_TRANSLATE_NOOP("state", "Do not disturb");
        case core::profile_state::invisible:
            return QT_TRANSLATE_NOOP("state", "Invisible");
        case core::profile_state::away:
            return QT_TRANSLATE_NOOP("state", "Away");
        case core::profile_state::offline:
            return QT_TRANSLATE_NOOP("state", "Offline");
        default:
            im_assert(!"unknown core::profile_state value");
            return QString();
        }
    }

    Data::FString makeDraftText(const QString& _aimId)
    {
        auto draftText = Logic::getRecentsModel()->getDlgState(_aimId).draftText_;
        Utils::elideText(draftText, MAX_TOOLTIP_TEXT_LEN);
        Data::FString draftMessage = QT_TRANSLATE_NOOP("contact_list", "Draft");
        core::data::format::builder formats;
        formats %= core::data::range_format(core::data::format_type::bold, { 0, draftMessage.string().length() });
        draftMessage.setFormatting(formats.finalize());
        Data::FString::Builder builder;
        builder %= draftMessage;
        builder %= qsl("\n\"");
        builder %= draftText;
        builder %= qsl("\"");
        return builder.finalize();
    }
}

int32_t Logic::AbstractItemDelegateWithRegim::avatarProxyFlags() const
{
    int32_t flags = 0;
    if (replaceFavorites_)
        flags |= Logic::AvatarStorageProxy::ReplaceFavorites;

    return flags;
}

int32_t Logic::AbstractItemDelegateWithRegim::friendlyProxyFlags() const
{
    int32_t flags = 0;
    if (replaceFavorites_)
        flags |= Logic::FriendlyContainerProxy::ReplaceFavorites;

    return flags;
}
