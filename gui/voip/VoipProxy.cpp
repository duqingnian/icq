#include "stdafx.h"
#include "VoipProxy.h"
#include "../core_dispatcher.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../my_info.h"
#include "../cache/avatars/AvatarStorage.h"
#include "../controls/TextEmojiWidget.h"
#include "../controls/GeneralDialog.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/utils.h"
#include "../utils/features.h"
#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/sounds/SoundsManager.h"
#include "../../core/Voip/VoipManagerDefines.h"
#include "../../core/Voip/VoipSerialization.h"
#include "../main_window/MainPage.h"
#include "../main_window/MainWindow.h"
#include "main_window/history_control/HistoryControlPage.h"
#include "media/permissions/MediaCapturePermissions.h"
#include "MicroAlert.h"
#include "VideoWindow.h"
#include <functional>
#ifdef __APPLE__
#include "macos/macosspec.h"
#endif

namespace voip_proxy
{
    const std::string kCallType_NONE = "none";
};

namespace
{
    constexpr int kIncludeWatermark = 0;
    constexpr int kIncludeCameraOffStatus = 1;

    constexpr int kDefaultImageScale = 100;
    constexpr int kMaximumImageScale = 200;
    constexpr int kAvatarVideoSize = 160;
    constexpr int kAvatarLargeSize = 360;
    constexpr QSize kVCSAvatarImageSize(1280, 720);

    constexpr QImage::Format kImageFormat = QImage::Format_RGBA8888;

    void addTextToCollection(Ui::gui_coll_helper& collection, const std::string& name, const QString& text, const QColor& penColor, const QFont& font)
    {
        QFontMetrics fm(font);
        const QSize textSz = fm.size(Qt::TextSingleLine, text);

        int nickW = textSz.width();
        int nickH = textSz.height();

        // Made it even
        if (nickW % 2 == 1)
            nickW++;
        if (nickH % 2 == 1)
            nickH++;

        im_assert(nickW % 2 == 0 && nickH % 2 == 0);
        if (nickW && nickH)
        {
            QPainter painter;

            QImage pm(nickW, nickH, kImageFormat);
            pm.fill(Qt::transparent);

            const QPen pen(penColor);

            painter.begin(&pm);
            painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
            painter.setCompositionMode(QPainter::CompositionMode_Plus);

            painter.setPen(pen);
            painter.setFont(font);
            if (textSz.width() > nickW)
                painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignLeft | Qt::AlignVCenter, text);
            else
                painter.drawText(QRect(0, 0, nickW, nickH), Qt::TextSingleLine | Qt::AlignCenter, text);

    #ifdef _DEBUG
            painter.drawRect(QRect(0, 0, nickW - 1, nickH - 1));
    #endif
            painter.end();

            QImage image = pm;

            auto dataSize = image.sizeInBytes();
            auto dataPtr = image.bits();
            auto sz = image.size();

            if (dataSize && dataPtr)
            {
                core::ifptr<core::istream> stream(collection->create_stream());
                stream->write(dataPtr, dataSize);

                collection.set_value_as_stream(name, stream.get());
                collection.set_value_as_int(name + "_height", sz.height());
                collection.set_value_as_int(name + "_width", sz.width());
            }
        }
    }

    const auto getIndex = [](int _reason)
    {
        if (_reason == TR_BLOCKED_BY_CALLEE_PRIVACY)
            return voip_proxy::TermincationReasonGroup::PRIVACY;
        if (_reason == TR_BLOCKED_BY_CALLER_IS_STRANGER || _reason == TR_CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA)
            return voip_proxy::TermincationReasonGroup::INVALID_CALLER;
        return voip_proxy::TermincationReasonGroup::UNKNOWN;
    };

    const auto reasonHeader = [](voip_proxy::TermincationReasonGroup _reason)
    {
        QString text;
        if (_reason == voip_proxy::TermincationReasonGroup::PRIVACY)
            text = QString::fromUtf8("\xF0\x9F\x95\xB6") % QChar::Space % QT_TRANSLATE_NOOP("popup_window", "You cannot call these users because of privacy settings:");
        else if (_reason == voip_proxy::TermincationReasonGroup::INVALID_CALLER)
            text = QString::fromUtf8("\xE2\x9B\x94") % QChar::Space % QT_TRANSLATE_NOOP("popup_window", "You cannot call these users. Try to write them:");
        else
            text = QT_TRANSLATE_NOOP("popup_window", "Unknown error connecting to the following contacts:");
        return text;
    };
}

voip_proxy::VoipController::VoipController(Ui::core_dispatcher& _dispatcher)
    : dispatcher_(_dispatcher)
    , callTimeTimer_(this)
{
    connect(&callTimeTimer_, &QTimer::timeout, this, &VoipController::_updateCallTime, Qt::QueuedConnection);
    callTimeTimer_.setInterval(1000);

    const std::vector<unsigned> codePoints
    {
        0xF09F8EA9,
        0xF09F8FA0,
        0xF09F92A1,
        0xF09F9AB2,
        0xF09F8C8D,
        0xF09F8D8C,
        0xF09F8D8F,
        0xF09F909F,
        0xF09F90BC,
        0xF09F928E,
        0xF09F98BA,
        0xF09F8CB2,
        0xF09F8CB8,
        0xF09F8D84,
        0xF09F8D90,
        0xF09F8E88,
        0xF09F90B8,
        0xF09F9180,
        0xF09F9184,
        0xF09F9191,
        0xF09F9193,
        0xF09F9280,
        0xF09F9494,
        0xF09F94A5,
        0xF09F9A97,
        0xE2AD90,
        0xE28FB0,
        0xE29ABD,
        0xE29C8F,
        0xE29882,
        0xE29C82,
        0xE29DA4,
    };

    voipEmojiManager_.addMap(64, 64, ":/emoji/secure_emoji_64.png", codePoints, 64);
    voipEmojiManager_.addMap(80, 80, ":/emoji/secure_emoji_80.png", codePoints, 80);
    voipEmojiManager_.addMap(96, 96, ":/emoji/secure_emoji_96.png", codePoints, 96);
    voipEmojiManager_.addMap(128, 128, ":/emoji/secure_emoji_128.png", codePoints, 128);
    voipEmojiManager_.addMap(160, 160, ":/emoji/secure_emoji_160.png", codePoints, 160);
    voipEmojiManager_.addMap(192, 192, ":/emoji/secure_emoji_192.png", codePoints, 192);
}

voip_proxy::VoipController::~VoipController()
{
    callTimeTimer_.stop();
}

void voip_proxy::VoipController::_updateCallTime()
{
    callTimeElapsed_ += 1;
    Q_EMIT onVoipCallTimeChanged(callTimeElapsed_, true);
}

void voip_proxy::VoipController::setAvatars(int _size, const char*_contact, bool forPreview)
{
    auto sendAvatar = [this](int _size, const char*_contact, bool forPreview, int userBitmapParamBitSet, voip_manager::AvatarThemeType theme)
    {
        Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);

        _prepareUserBitmaps(collection, _contact, _size, userBitmapParamBitSet, theme);
        if (forPreview)
            collection.set_value_as_string("contact", PREVIEW_RENDER_NAME);

        dispatcher_.post_message_to_core("voip_call", collection.get());
    };

    sendAvatar(_size, _contact, forPreview, AVATAR_ROUND | USER_NAME_ONE_IS_BIG, voip_manager::MAIN_ONE_BIG);
}

std::string voip_proxy::VoipController::formatCallName(const std::vector<std::string>& _names, const char* _clip)
{
    std::string name;
    for (unsigned ix = 0; ix < _names.size(); ix++)
    {
        const std::string& nt = _names[ix];
        im_assert(!nt.empty());
        if (nt.empty())
            continue;

        if (!name.empty())
        {
            if (ix == _names.size() - 1 && _clip)
            {
                name += ' ';
                name += _clip;
                name += ' ';
            }
            else
            {
                name += ", ";
            }
        }
        name += nt;
    }
    im_assert(!name.empty() || _names.empty());
    return name;
}

void voip_proxy::VoipController::setRequestSettings()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "update");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_prepareUserBitmaps(Ui::gui_coll_helper& _collection, const std::string& _contact,
    int _size, int userBitmapParamBitSet, voip_manager::AvatarThemeType theme)
{
    im_assert(!_contact.empty() && _size);
    if (_contact.empty() || !_size)
        return;

    _collection.set_value_as_string("type", "converted_avatar");
    _collection.set_value_as_string("contact", _contact);
    _collection.set_value_as_int("size", _size);
    _collection.set_value_as_int("theme", theme);

    const auto contact = QString::fromStdString(_contact);
    bool needAvatar = (userBitmapParamBitSet & (AVATAR_ROUND | AVATAR_SQUARE_BACKGROUND | AVATAR_SQUARE_WITH_LETTER));
    bool isSelf = Ui::MyInfo()->aimId() == contact;

    QString friendContact = isSelf ? Ui::MyInfo()->friendly() : Logic::GetFriendlyContainer()->getFriendly(contact);
    if (friendContact.isEmpty())
        friendContact = contact;

    QPixmap realAvatar;
    bool defaultAvatar = false;
    if (needAvatar)
    {
        if (!isSelf)
        {
            Utils::loadPixmap(qsl(":/voip/videoconference"), realAvatar);
        }
        else
        {
            realAvatar = Logic::GetAvatarStorage()->Get(contact, friendContact, _size, defaultAvatar, false);
        }
    }

    auto packAvatar = [&_collection](const QImage& image, const std::string& prefix)
    {
        const auto dataSize = image.sizeInBytes();
        const auto dataPtr = image.bits();

        im_assert(dataSize && dataPtr);
        if (dataSize && dataPtr)
        {
            core::ifptr<core::istream> avatar_stream(_collection->create_stream());
            avatar_stream->write(dataPtr, dataSize);
            _collection.set_value_as_stream(prefix + "avatar", avatar_stream.get());
            _collection.set_value_as_int(prefix + "avatar_height", image.height());
            _collection.set_value_as_int(prefix + "avatar_width", image.width());
        }
    };

    if (!realAvatar.isNull())
    {
        const auto rawAvatar = realAvatar.toImage();
        if (needAvatar)
        {
            enum AvatarType {AT_AVATAR_SQUARE_BACKGROUND = 0, AT_AVATAR_SQUARE_WITH_LETTER, AT_AVATAR_ROUND};

            auto makeAvatar = [_size, &rawAvatar, packAvatar, vcsModifyAvatar = (isSelf)](AvatarType avatarType)
            {
                bool squareBackground = (avatarType == AT_AVATAR_SQUARE_BACKGROUND);
                bool squareLetter = (avatarType == AT_AVATAR_SQUARE_WITH_LETTER);
                bool roundAvatar = (avatarType == AT_AVATAR_ROUND);
                bool squareAvatar = (squareBackground || squareLetter);

                const int avatarSize = (roundAvatar && !vcsModifyAvatar) ? Utils::scale_bitmap_with_value(voip_manager::kAvatarDefaultSize) : _size;
                const auto imageSize = (roundAvatar && vcsModifyAvatar) ? kVCSAvatarImageSize : QSize(avatarSize, avatarSize);
                const auto posX = (roundAvatar && vcsModifyAvatar) ? (imageSize.width() - avatarSize) / 2 : 0;
                const auto posY = (roundAvatar && vcsModifyAvatar) ? (imageSize.height() - avatarSize) / 2 : 0;
                const auto avatarRect = QRect(posX, posY, avatarSize, avatarSize);

                QImage image(imageSize, kImageFormat);
                image.fill(Qt::transparent);

                QPainter painter(&image);
                painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
                painter.setPen(Qt::NoPen);
                painter.setBrush(Qt::NoBrush);

                if (roundAvatar)
                {
                    QPainterPath path;
                    path.addEllipse(avatarRect);
                    painter.fillPath(path, QBrush(Qt::white));
                    painter.setCompositionMode(QPainter::CompositionMode_SourceAtop);
                }

                painter.drawImage(avatarRect, rawAvatar);

                if (squareAvatar)
                    painter.fillRect(image.rect(), QColor(0, 0, 0, 128));

                packAvatar(image, "");
            };

            if (userBitmapParamBitSet & AVATAR_ROUND)
                makeAvatar(AT_AVATAR_ROUND);
        }
    }
}

void voip_proxy::VoipController::handlePacket(core::coll_helper& _collParams)
{
    const std::string sigType = _collParams.get_value_as_string("sig_type");
    if (sigType == "video_window_show")
    {
        const bool enable = _collParams.get_value_as_bool("param");
        Q_EMIT onVoipShowVideoWindow(enable);
    } else if (sigType == "device_vol_changed")
    {
        int volumePercent = _collParams.get_value_as_int("volume_percent");
        const std::string deviceType = _collParams.get_value_as_string("device_type");

        volumePercent = std::min(100, std::max(volumePercent, 0));
        Q_EMIT onVoipVolumeChanged(deviceType, volumePercent);
    } else if (sigType == "voip_reset_complete")
    {
        voipIsKilled_ = true;
        Q_EMIT onVoipResetComplete();
    } else if (sigType == "call_peer_list_changed")
    {
        voip_manager::ContactsList contacts;
        contacts << _collParams;

        if (contacts.isActive)
        {
            vcsWebinar_ = contacts.is_webinar;
            vcsConferenceUrl_ = contacts.conference_url;
            activePeerList_ = contacts;
        }

        Q_EMIT onVoipCallNameChanged(contacts);
    } else if (sigType == "voip_window_add_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;
        Q_EMIT onVoipWindowAddComplete(hwnd);
    } else if (sigType == "voip_window_remove_complete")
    {
        intptr_t hwnd;
        hwnd << _collParams;
        Q_EMIT onVoipWindowRemoveComplete(hwnd);
    } else if (sigType == "call_in_accepted")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        auto it = inCallsList_.find(contactEx.contact.call_id);
        if (it != inCallsList_.end())
            inCallsList_.erase(it);

        callType_ = contactEx.call_type;
        chatId_ = contactEx.chat_id;
        haveEstablishedConnection_ = true;
        Q_EMIT onVoipCallIncomingAccepted(contactEx.contact.call_id);
    } else if (sigType == "device_list_changed")
    {
        const int deviceCount = _collParams.get_value_as_int("count");
        const DeviceType deviceType = (DeviceType)_collParams.get_value_as_int("type");
        auto& devices = devices_[deviceType];

        devices.clear();
        devices.reserve(deviceCount);

        if (deviceCount > 0)
        {
            for (int ix = 0; ix < deviceCount; ix++)
            {
                std::stringstream ss;
                ss << "device_" << ix;

                auto device = _collParams.get_value_as_collection(ss.str().c_str());
                im_assert(device);
                if (device)
                {
                    core::coll_helper device_helper(device, false);

                    device_desc desc;
                    desc.name = device_helper.get_value_as_string("name");
                    desc.uid  = device_helper.get_value_as_string("uid");
                    desc.video_dev_type = (EvoipVideoDevTypes)device_helper.get_value_as_int("video_capture_type");

                    const std::string type = device_helper.get_value_as_string("device_type");
                    desc.dev_type = type == "audio_playback" ? kvoipDevTypeAudioPlayback :
                        type == "video_capture"  ? kvoipDevTypeVideoCapture :
                        type == "audio_capture"  ? kvoipDevTypeAudioCapture :
                        kvoipDevTypeUndefined;

                    im_assert(desc.dev_type != kvoipDevTypeUndefined);
                    if (desc.dev_type != kvoipDevTypeUndefined)
                    {
                        devices.push_back(desc);
                    }
                }
            }
        }
        im_assert((deviceCount > 0 && !devices.empty()) || (!deviceCount && devices.empty()));

        if (deviceType == VideoCapturing)
            updateScreenList(devices);
        Q_EMIT onVoipDeviceListUpdated((EvoipDevTypes)(deviceType + 1), devices);
    } else if (sigType == "media_loc_params_changed")
    {
        localAudioEnabled_ = _collParams.get_value_as_bool("local_aud_en");
        localVideoEnabled_ = _collParams.get_value_as_bool("local_cam_en");
        localAudAllowed_ = _collParams.get_value_as_bool("local_aud_allowed");
        localCamAllowed_ = _collParams.get_value_as_bool("local_cam_allowed");
        localDesktopAllowed_ = _collParams.get_value_as_bool("local_desktop_allowed");
        localCameraEnabled_ = currentSelectedVideoDevice_.video_dev_type == kvoipDeviceCamera ? localVideoEnabled_ : localCameraEnabled_;
        Q_EMIT onVoipMediaLocalAudio(localAudioEnabled_);
        Q_EMIT onVoipMediaLocalVideo(localVideoEnabled_);
    } else if (sigType == "call_created")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        if (!haveEstablishedConnection_)
        {
            Q_EMIT Utils::InterConnector::instance().stopPttRecord();
            onStartCall(!contactEx.incoming);
        }
        if (!contactEx.incoming)
        {
            callType_ = contactEx.call_type;
            chatId_   = contactEx.chat_id;
            haveEstablishedConnection_ = true;
        }

        Q_EMIT onVoipCallCreated(contactEx);
        if (contactEx.incoming)
        {
            inCallsList_[contactEx.contact.call_id] = contactEx;
            Q_EMIT onVoipCallIncoming(contactEx.contact.call_id, contactEx.contact.contact, contactEx.call_type);
        }
        Ui::GetSoundsManager()->callInProgress(true);
    } else if (sigType == "call_destroyed")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;
        auto it = inCallsList_.find(contactEx.contact.call_id);
        if (it != inCallsList_.end())
            inCallsList_.erase(it);

        if (contactEx.current_call)
        {
            callTimeTimer_.stop();
            auto elapsed = callTimeElapsed_;

            callTimeElapsed_ = 0;
            haveEstablishedConnection_ = false;
            chatId_ = {};
            callType_ = voip_proxy::kCallType_NONE;
            vcsConferenceUrl_.clear();
            disconnectedPeers_.clear();
            activePeerList_ = {};
            onEndCall();

            Q_EMIT onVoipCallTimeChanged(callTimeElapsed_, false);

            voip_manager::CallEndStat stat(elapsed, contactEx.contact.contact);
            Q_EMIT onVoipCallEndedStat(stat);
        }
        Q_EMIT onVoipCallDestroyed(contactEx);
        Ui::GetSoundsManager()->callInProgress(false);

        QString text;
        QString title;
        const auto reason = contactEx.terminate_reason;
        switch (reason)
        {
        case TR_NOT_FOUND:
            title = QT_TRANSLATE_NOOP("popup_window", "Call declined");
            text = QT_TRANSLATE_NOOP("popup_window", "Sorry, something went wrong. Please, try again later");
            break;
        case TR_BLOCKED_BY_CALLER_IS_STRANGER:
        case TR_CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA:
            title = QT_TRANSLATE_NOOP("popup_window", "Call declined");
            text = QT_TRANSLATE_NOOP("popup_window", "You cannot call this contact. Write %1 to the chat and, if answered, you can call")
                .arg(Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(contactEx.contact.contact)));
            break;
        case TR_BLOCKED_BY_CALLEE_PRIVACY:
            title = QT_TRANSLATE_NOOP("popup_window", "Call declined");
            text = QT_TRANSLATE_NOOP("popup_window", "You cannot call this contact due to its privacy settings");
            break;
        case TR_BAD_URI:
            title = QT_TRANSLATE_NOOP("popup_window", "Some problems with your link");
            text = QT_TRANSLATE_NOOP("popup_window", "Some problems with your link. Please check it and try again or ask for a new link");
            break;
        case TR_NOT_AVAILABLE_NOW:
            title = QT_TRANSLATE_NOOP("popup_window", "Error");
            text = QT_TRANSLATE_NOOP("popup_window", "It's not possible to connect now, please, try again later");
            break;
        case TR_PARTICIPANTS_LIMIT_EXCEEDED:
            title = QT_TRANSLATE_NOOP("popup_window", "Error");
            if (isCallVCS())
            {
                text = QT_TRANSLATE_NOOP("popup_window", "Participants' limit exceeded");
            }
            else
            {
                text = QT_TRANSLATE_NOOP("popup_window", "You can not join the call. It already has") % ql1c(' ') % QString::number(maxVideoConferenceMembers()) % ql1c(' ')
                        % Utils::GetTranslator()->getNumberString(
                            maxVideoConferenceMembers(),
                            QT_TRANSLATE_NOOP3("popup_window", "participant", "1"),
                            QT_TRANSLATE_NOOP3("popup_window", "participants", "2"),
                            QT_TRANSLATE_NOOP3("popup_window", "participants", "5"),
                            QT_TRANSLATE_NOOP3("popup_window", "participants", "21"));
            }
            break;
        case TR_DURATION_LIMIT_EXCEEDED:
            title = QT_TRANSLATE_NOOP("popup_window", "Error");
            text = QT_TRANSLATE_NOOP("popup_window", "Duration limit exceeded");
            break;
        case TR_INTERNAL_ERROR:
        case TR_ALLOCATE_FAILED:
            title = QT_TRANSLATE_NOOP("popup_window", "Error");
            text = QT_TRANSLATE_NOOP("popup_window", "Sorry, something went wrong. Please, try again later");
            break;
        }

        if (reason == TR_BLOCKED_BY_CALLER_IS_STRANGER || reason == TR_BLOCKED_BY_CALLEE_PRIVACY || reason == TR_CALLER_MUST_BE_AUTHORIZED_BY_CAPCHA)
        {
            if (Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Write"),
                text,
                title,
                nullptr
            ))
            {
                openChat(QString::fromStdString(contactEx.contact.contact));
            }
        }
        else if (!text.isEmpty() || !title.isEmpty())
        {
            Utils::GetConfirmationWithOneButton(QT_TRANSLATE_NOOP("popup_window", "Ok"), text, title, nullptr);
        }

        if (contactEx.current_call && pendingCallStart_)
        {
            dispatcher_.post_message_to_core("voip_call", pendingCallStart_->get());
            pendingCallStart_.reset();
        }
    } else if (sigType == "call_connected")
    {
        voip_manager::ContactEx contactEx;
        contactEx << _collParams;

        Q_EMIT onVoipCallTimeChanged(callTimeElapsed_, true);

        if (!callTimeTimer_.isActive())
        {
            callTimeTimer_.start();
        }

        Q_EMIT onVoipCallConnected(contactEx);
    } else if (sigType == "media_rem_v_changed")
    {
        voip_manager::VideoEnable videoEnable;
        videoEnable << _collParams;
        Q_EMIT onVoipMediaRemoteVideo(videoEnable);
    }
    else if (sigType == "voice_detect")
    {
        const bool enabled = _collParams.get_value_as_bool("param");
        Q_EMIT onVoipVoiceEnable(enabled);
    } else if (sigType == "voice_vad_info")
    {
        std::vector<im::VADInfo> peers;
        peers << _collParams;
        Q_EMIT onVoipVadInfo(peers);
    } else if (sigType == "media_loc_v_device_changed")
    {
        voip_manager::device_description device;
        device << _collParams;

        device_desc deviceProxy;
        deviceProxy.uid = device.uid;
        deviceProxy.dev_type = device.type == AudioPlayback ? kvoipDevTypeAudioPlayback :
            device.type == VideoCapturing ? kvoipDevTypeVideoCapture :
            device.type == AudioRecording ? kvoipDevTypeAudioCapture :
            kvoipDevTypeUndefined;
        deviceProxy.video_dev_type = (EvoipVideoDevTypes)device.videoCaptureType;

        if (deviceProxy.dev_type == kvoipDevTypeVideoCapture)
        {
            currentSelectedVideoDevice_ = deviceProxy;
            Q_EMIT onVoipVideoDeviceSelected(deviceProxy);
        }
    } else if (sigType == "conf_peer_disconnected")
    {
        voip_manager::ConfPeerInfoV peers;
        peers << _collParams;

        for (const auto& peerInfo : peers)
        {
            auto& reasonKeeper = disconnectedPeers_[getIndex(peerInfo.terminate_reason)];
            if (std::find(reasonKeeper.begin(), reasonKeeper.end(), peerInfo.peerId) == reasonKeeper.end())
                reasonKeeper.push_back(peerInfo.peerId);
        }
        updateDisconnectedPeers();
    } else if (sigType == "hide_ctrls_when_remote_sharing")
    {
        const bool hide = _collParams.get_value_as_bool("enable");
        hideControlsWhenRemDesktopSharing_ = hide;
        Q_EMIT onVoipHideControlsWhenRemDesktopSharing(hide);
    }
}

void voip_proxy::VoipController::updateActivePeerList()
{
    Q_EMIT onVoipCallNameChanged(activePeerList_);
}

void voip_proxy::VoipController::setHangup()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_stop");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchAPlaybackMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute_switch");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setSwitchACaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", false);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_setSwitchVCaptureMute()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_media_switch");
    collection.set_value_as_bool("video", true);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::updateDisconnectedPeers()
{
    QString text;
    int counter = 0;

    for (const auto&[reason, peers] : disconnectedPeers_)
    {
        if (!text.isEmpty())
        {
            text += QChar::LineFeed;
            text += QChar::LineFeed;
        }

        text += reasonHeader(reason);
        text += QChar::Space;
        for (const auto& p : peers)
        {
            text += Logic::GetFriendlyContainer()->getFriendly(QString::fromStdString(p));
            text += ql1s(", ");
            ++counter;
        }

        text.chop(2);
    }
    disconnectedPeers_.clear();

    if (!text.isEmpty())
    {
        if (auto wnd = Utils::InterConnector::instance().getMainWindow())
            wnd->activate();

        const QString title = QT_TRANSLATE_NOOP("popup_window", "You cannot call") % ql1c(' ') % QString::number(counter) % ql1c(' ')
            % Utils::GetTranslator()->getNumberString(
                counter,
                QT_TRANSLATE_NOOP3("popup_window", "contact", "1"),
                QT_TRANSLATE_NOOP3("popup_window", "contacts", "2"),
                QT_TRANSLATE_NOOP3("popup_window", "contacts", "5"),
                QT_TRANSLATE_NOOP3("popup_window", "contacts", "21")
            );

        Utils::GetConfirmationWithOneButton(
            QT_TRANSLATE_NOOP("popup_window", "OK"),
            text,
            title,
            nullptr
        );
    }

    text.clear();
}

void voip_proxy::VoipController::setSwitchVCaptureMute()
{
    if (currentSelectedVideoDevice_.video_dev_type == kvoipDeviceDesktop)
    {
        voip_proxy::device_desc lastSelectedCamera = activeDevices_[kvoipDevTypeVideoCapture];
        setActiveDevice(lastSelectedCamera);
    } else
        _setSwitchVCaptureMute();
}

void voip_proxy::VoipController::setMuteSounds(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_sounds_mute");
    collection.set_value_as_bool("mute", _mute);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setActiveDevice(const voip_proxy::device_desc& _description, bool _force_reset)
{
    std::string devType;
    switch (_description.dev_type)
    {
    case kvoipDevTypeAudioPlayback:
        devType = "audio_playback";
        break;
    case kvoipDevTypeAudioCapture:
        devType = "audio_capture";
        break;
    case kvoipDevTypeVideoCapture:
        devType = "video_capture";
        break;
    case kvoipDevTypeUndefined:
    default:
        im_assert(!"unexpected device type");
        return;
    };

    if (!_description.uid.empty() && !(kvoipDevTypeVideoCapture == _description.dev_type && kvoipDeviceDesktop == _description.video_dev_type))
        activeDevices_[_description.dev_type] = _description;

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "device_change");
    collection.set_value_as_string("dev_type", devType);
    collection.set_value_as_string("uid", _description.uid);
    collection.set_value_as_bool("force_reset", _force_reset);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAcceptCall(const char* call_id, bool video)
{
    im_assert(call_id);
    if (!call_id)
        return;
    video = checkPermissions(true, video);
    video &= localCamPermission_;

    setAvatars(kAvatarLargeSize, Ui::MyInfo()->aimId().toStdString().c_str(), true);
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_accept");
    collection.set_value_as_string("mode", video ? "video" : "audio");
    collection.set_value_as_string("call_id", call_id);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::voipReset()
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_reset");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setDecline(const char* call_id, const char* _contact, bool _busy, bool conference)
{
    im_assert(_contact);
    if (!_contact)
        return;

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_decline");
    collection.set_value_as_string("mode", _busy ? "busy" : "not_busy");
    collection.set_value_as_string("conference", conference ? "true" : "false");
    collection.set_value_as_string("call_id", call_id);
    collection.set_value_as_string("contact", _contact);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

bool voip_proxy::VoipController::checkPermissions(bool _audio, bool _video, bool _doRequest)
{
    if (_audio)
    {
        const auto restartDevice = [this]()
        {
            // restart audio device if permission just granted
            voip_proxy::device_desc description = activeDevices_[kvoipDevTypeAudioCapture];
            setActiveDevice(description, true);
        };
        auto p = media::permissions::checkPermission(media::permissions::DeviceType::Microphone);
        const auto localAudPermission = (media::permissions::Permission::Allowed == p);
        const auto needReset = localAudPermission != localAudPermission_;
        localAudPermission_ = localAudPermission;

        if (_doRequest && !localAudPermission_)
        {
            if (media::permissions::Permission::NotDetermined == p)
            {
                media::permissions::requestPermission(media::permissions::DeviceType::Microphone, [this, restartDevice](bool granted)
                    {
                        if (granted)
                            restartDevice();
                        localAudPermission_ = granted;
                        Q_EMIT onVoipMediaLocalAudio(localAudioEnabled_);
                    });
            }
            else
            {
                Utils::showPermissionDialog(Utils::InterConnector::instance().getMainWindow());
            }
        }
        else
        {
            if (needReset && localAudPermission_)
                restartDevice();
            Q_EMIT onVoipMediaLocalAudio(localAudioEnabled_);
        }
    }
    if (_video)
    {
        auto p = media::permissions::checkPermission(media::permissions::DeviceType::Camera);
        _video = localCamPermission_ = (media::permissions::Permission::Allowed == p);
        if (_doRequest && media::permissions::Permission::NotDetermined == p)
        {
            _video = true;
            media::permissions::requestPermission(media::permissions::DeviceType::Camera, [this](bool granted){
                localCamPermission_ = granted;
                if (granted)
                {
                    _setSwitchVCaptureMute();
                } else
                {
                    Q_EMIT onVoipMediaLocalVideo(localVideoEnabled_);
                }
            });
        }
    }
    return _video;
}

bool voip_proxy::VoipController::setStartCall(const std::vector<QString>& _contacts, bool _video, bool _attach)
{
    im_assert(!_contacts.empty());
    if (_contacts.empty())
        return false;

    pendingCallStart_.reset();
    CheckActiveCallResult check = NoActiveCall;
    if (haveEstablishedConnection_ && !_attach)
    {
        std::string contact = (1 == _contacts.size()) ? _contacts[0].toStdString() : std::string();
        check = checkActiveCall(contact);
        if (Cancelled == check || SameCall == check)
            return SameCall == check;
    }

    if (!_attach)
        _video = checkPermissions(true, _video);
    _video &= localCamPermission_;

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");

    core::ifptr<core::iarray> contactsArray(collection->create_array());
    contactsArray->reserve(_contacts.size());
    for (const auto& contact : _contacts)
        contactsArray->push_back(collection.create_qstring_value(contact).get());
    collection.set_value_as_array("contacts", contactsArray.get());

    collection.set_value_as_string("call_type", "normal");
    collection.set_value_as_bool("video", _video);
    collection.set_value_as_bool("attach", _attach);

    if (CallEnded == check)
        pendingCallStart_.reset(new Ui::gui_coll_helper(collection));
    else
        dispatcher_.post_message_to_core("voip_call", collection.get());
    return true;
}

bool voip_proxy::VoipController::setStartChatRoomCall(const QStringView _chatId, const std::vector<QString>& _contacts, bool _video)
{
    im_assert(!_contacts.empty());
    pendingCallStart_.reset();
    std::string cId = _chatId.toUtf8().constData();
    for (auto call: inCallsList_)
    {
        if (call.second.chat_id == cId)
        {
            Q_EMIT onVoipCallDestroyed(call.second);
            setDecline(call.second.contact.call_id.c_str(), call.second.contact.contact.c_str(), false);
            break;
        }
    }

    CheckActiveCallResult check = checkActiveCall(cId);
    if (Cancelled == check || SameCall == check)
        return SameCall == check;
    _video = checkPermissions(true, _video);
    _video &= localCamPermission_;

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");

    core::ifptr<core::iarray> contactsArray(collection->create_array());
    contactsArray->reserve(_contacts.size() + 1);
    contactsArray->push_back(collection.create_qstring_value(_chatId).get());
    for (const auto& contact : _contacts)
        contactsArray->push_back(collection.create_qstring_value(contact).get());
    collection.set_value_as_array("contacts", contactsArray.get());

    collection.set_value_as_string("call_type", "pinned_room");
    collection.set_value_as_bool("video", _video);

    if (CallEnded == check)
        pendingCallStart_.reset(new Ui::gui_coll_helper(collection));
    else
        dispatcher_.post_message_to_core("voip_call", collection.get());
    return true;
}

bool voip_proxy::VoipController::setStartChatRoomCall(const QStringView _chatId, bool _video)
{
    return setStartChatRoomCall(_chatId, std::vector<QString>(), _video);
}

void voip_proxy::VoipController::setStartVCS(const QString &_urlConference)
{
    pendingCallStart_.reset();
    CheckActiveCallResult check = checkActiveCall(_urlConference.toStdString());
    if (Cancelled == check || SameCall == check)
        return;
    checkPermissions(true, false);

    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_call_start");

    core::ifptr<core::iarray> contactsArray(collection->create_array());
    contactsArray->reserve(1);
    contactsArray->push_back(collection.create_qstring_value(_urlConference).get());
    collection.set_value_as_array("contacts", contactsArray.get());

    collection.set_value_as_string("call_type", "vcs");

    if (CallEnded == check)
        pendingCallStart_.reset(new Ui::gui_coll_helper(collection));
    else
        dispatcher_.post_message_to_core("voip_call", collection.get());

    setAvatars(kAvatarLargeSize, Ui::MyInfo()->aimId().toStdString().c_str(), true);
}

void voip_proxy::VoipController::setWindowAdd(quintptr _hwnd, const char *call_id, bool _primaryWnd, bool _incomeingWnd, int _panelHeight)
{
    if (!_hwnd)
        return;
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_add_window");
    collection.set_value_as_string("call_id", call_id);
    collection.set_value_as_double("scale", Utils::getScaleCoefficient() * Utils::scale_bitmap_ratio());
    collection.set_value_as_bool("mode", _primaryWnd);
    collection.set_value_as_bool("incoming_mode", _incomeingWnd);
    collection.set_value_as_int64("handle", _hwnd);

    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setWindowRemove(quintptr _hwnd)
{
    if (!_hwnd)
        return;
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_remove_window");
    collection.set_value_as_int64("handle", _hwnd);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::setAPlaybackMute(bool _mute)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "audio_playback_mute");
    collection.set_value_as_string("mute", _mute ? "on" : "off");
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

void voip_proxy::VoipController::_checkIgnoreContact(const QString& _contact)
{
    std::string contactName = _contact.toStdString();
    if (std::any_of(
            activePeerList_.contacts.cbegin(), activePeerList_.contacts.cend(), [&contactName](const voip_manager::Contact& _contact)
            {
                return _contact.contact == contactName;
            }
       ))
    {
        setHangup();
    }
}

void voip_proxy::VoipController::loadSettings(std::function<void(voip_proxy::device_desc &description)> callback)
{
    // read voip settings and apply devices config
    auto setActiveDeviceByType = [this, &callback](const voip_proxy::EvoipDevTypes& type, QString settingsName, QString defaultFlagSettingName)
    {
        QString val = Ui::get_gui_settings()->get_value<QString>(settingsName, QString());
        bool applyDefaultDevice = !defaultFlagSettingName.isEmpty() ? Ui::get_gui_settings()->get_value<bool>(defaultFlagSettingName, false) : false;
        voip_proxy::device_desc description;
        description.uid = (applyDefaultDevice || val.isEmpty()) ? DEFAULT_DEVICE_UID : val.toStdString();
        description.dev_type = type;
        if (!settingsLoaded_)
            setActiveDevice(description);
        callback(description);
    };
    setActiveDeviceByType(voip_proxy::kvoipDevTypeAudioPlayback, ql1s(settings_speakers),   ql1s(settings_speakers_is_default));
    setActiveDeviceByType(voip_proxy::kvoipDevTypeAudioCapture,  ql1s(settings_microphone), ql1s(settings_microphone_is_default));
    setActiveDeviceByType(voip_proxy::kvoipDevTypeVideoCapture,  ql1s(settings_webcam),     QString());
    settingsLoaded_ = true;
}

void voip_proxy::VoipController::onStartCall(bool _bOutgoing)
{
#ifdef _WIN32
    trackDevices(false); // WaveOut do not supports default device change monitoring, trigger it manually on start call
#endif
    // Change volume from application settings only for incomming calls.
    if (!_bOutgoing)
    {
        setMuteSounds(false);
    }
#ifdef __APPLE__
    // Pause iTunes if we have call.
    iTunesWasPaused_ = iTunesWasPaused_ || platform_macos::pauseiTunes();
#endif
    connect(Logic::getContactListModel(), &Logic::ContactListModel::ignore_contact, this, &voip_proxy::VoipController::_checkIgnoreContact);
    connect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &voip_proxy::VoipController::avatarChanged);
}

void voip_proxy::VoipController::onEndCall()
{
#ifdef __APPLE__
    if (iTunesWasPaused_)
    {   // Resume iTunes if we paused it.
        platform_macos::playiTunes();
        iTunesWasPaused_ = false;
    }
#endif
    disconnect(Logic::getContactListModel(), &Logic::ContactListModel::ignore_contact, this, &voip_proxy::VoipController::_checkIgnoreContact);
    disconnect(Logic::GetAvatarStorage(), &Logic::AvatarStorage::avatarChanged, this, &voip_proxy::VoipController::avatarChanged);
}

bool voip_proxy::VoipController::isResetCallDialogActive() const
{
    return isResetDialogActive_;
}

const std::vector<voip_proxy::device_desc>& voip_proxy::VoipController::deviceList(EvoipDevTypes type) const
{
    auto arrayIndex = type - 1;
    if (arrayIndex >= 0 && arrayIndex < sizeof(devices_) / sizeof(devices_[0]))
    {
        return devices_[arrayIndex];
    }
    im_assert(false && "Wrong device type");
    static const std::vector<voip_proxy::device_desc> emptyList;
    return emptyList;
}

std::optional<voip_proxy::device_desc> voip_proxy::VoipController::activeDevice(EvoipDevTypes type) const
{
    if (const auto it = activeDevices_.find(type); it != activeDevices_.end())
        return it->second;
    return std::nullopt;
}

void voip_proxy::VoipController::avatarChanged(const QString& aimId)
{
    auto currentContact = aimId.toStdString();
    if (aimId == Ui::MyInfo()->aimId())
    {
        setAvatars(kAvatarLargeSize, currentContact.c_str(), true);
        return;
    }
}

const std::vector<voip_manager::Contact>& voip_proxy::VoipController::currentCallContacts() const
{
    return activePeerList_.contacts;
}

bool voip_proxy::VoipController::isVoipKilled() const
{
    return voipIsKilled_;
}

bool voip_proxy::VoipController::isConnectionEstablished() const
{
    return haveEstablishedConnection_;
}

bool voip_proxy::VoipController::hasEstablishCall() const
{
    return kCallType_NONE != callType_;
}

int voip_proxy::VoipController::maxVideoConferenceMembers() const
{
    return Features::getVoipCallUserLimit();
}

int voip_proxy::VoipController::maxUsersWithVideo() const
{
    return Features::getVoipVideoUserLimit();
}

int voip_proxy::VoipController::lowerBoundForBigConference() const
{
    return Features::getVoipBigConferenceBoundary();
}

void voip_proxy::VoipController::openChat(const QString& contact)
{
    if (auto wnd = Utils::InterConnector::instance().getMainWindow())
    {
        wnd->raise();
        wnd->activate();
        wnd->showMessengerPage();
    }
    Utils::InterConnector::instance().openDialog(contact);
}

void voip_proxy::VoipController::switchShareScreen(voip_proxy::device_desc const * _description)
{
    if (_description && currentSelectedVideoDevice_.video_dev_type == kvoipDeviceCamera)
    {
        setActiveDevice(*_description);
        if (!localVideoEnabled_)
            _setSwitchVCaptureMute();
    } else
    {
        if (localVideoEnabled_ && !localCameraEnabled_)
            _setSwitchVCaptureMute();
        voip_proxy::device_desc lastSelectedCamera = activeDevices_[kvoipDevTypeVideoCapture];
        setActiveDevice(lastSelectedCamera);
    }
}

void voip_proxy::VoipController::updateScreenList(const std::vector<voip_proxy::device_desc>& _devices)
{
    screenList_.clear();
    for (voip_proxy::device_desc device : _devices)
    {
        if (device.dev_type == voip_proxy::kvoipDevTypeVideoCapture && device.video_dev_type == voip_proxy::kvoipDeviceDesktop)
            screenList_.push_back(device);
    }
}

void voip_proxy::VoipController::trackDevices(bool track_video)
{
    if (!settingsLoaded_)
        return;
    voip_proxy::device_desc description;
    description = activeDevices_[kvoipDevTypeAudioPlayback];
    setActiveDevice(description);

    description = activeDevices_[kvoipDevTypeAudioCapture];
    setActiveDevice(description);
    if (track_video && currentSelectedVideoDevice_.video_dev_type != kvoipDeviceDesktop)
    {
        description = activeDevices_[kvoipDevTypeVideoCapture];
        setActiveDevice(description);
    }
}

voip_proxy::CheckActiveCallResult voip_proxy::VoipController::checkActiveCall(const std::string& _contact)
{
    if (isConnectionEstablished())
    {
        const auto sameRoom = isCallPinnedRoom() && getChatId() == _contact;
        const auto sameVCS = isCallVCS() && vcsConferenceUrl_ == _contact && chatId_ == _contact;
        const auto sameVCSLink = isCallVCSLink() && chatId_ == _contact;
        const auto sameContact = callType_ == voip::kCallType_NORMAL && 1 == activePeerList_.contacts.size() && activePeerList_.contacts[0].contact == _contact;

        if (!sameRoom && !sameContact && !sameVCS && !sameVCSLink)
        {
            if (Ui::GeneralDialog* instance = Ui::GeneralDialog::activeInstance())
            {
                if (Utils::hasAncestorOf<Ui::VideoWindow*>(instance))
                {
                    disconnect(this, &voip_proxy::VoipController::onVoipWindowRemoveComplete, instance, &Ui::GeneralDialog::reject);
                    instance->reject();
                }
            }
            QScopedValueRollback guard(isResetDialogActive_, true);
            QString text;
            auto page = Utils::InterConnector::instance().getHistoryPage(QString::fromStdString(_contact));
            if (page && page->chatRoomCallParticipantsCount() > 0)
                text = QT_TRANSLATE_NOOP("popup_window", "Do you want to join the call? Current call will be ended");
            else
                text = QT_TRANSLATE_NOOP("popup_window", "Do you want to start a new call? Current call will be ended");

            const auto endCurrentCall = Utils::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("popup_window", "No"),
                                                                             QT_TRANSLATE_NOOP("popup_window", "Yes"),
                                                                             text,
                                                                             QString(), nullptr);
            if (!endCurrentCall)
                return Cancelled;
            setHangup();
            return CallEnded;
        } else
            return SameCall;
    }
    return NoActiveCall;
}

void voip_proxy::VoipController::notifyDevicesChanged(bool audio)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_devices_changed");
    collection.set_value_as_bool("audio", audio);
    dispatcher_.post_message_to_core("voip_call", collection.get());
    trackDevices();
}

const QList<voip_proxy::device_desc>& voip_proxy::VoipController::screenList() const
{
    return screenList_;
}

void voip_proxy::VoipController::setRenderResolution(std::string_view _contact, int _width, int _height)
{
    Ui::gui_coll_helper collection(dispatcher_.create_collection(), true);
    collection.set_value_as_string("type", "voip_peer_resolution_changed");
    collection.set_value_as_string("contact", _contact);
    collection.set_value_as_int("width", _width);
    collection.set_value_as_int("height", _height);
    dispatcher_.post_message_to_core("voip_call", collection.get());
}

voip_proxy::VoipEmojiManager::VoipEmojiManager()
    : activeMapId_(0)
{
}

voip_proxy::VoipEmojiManager::~VoipEmojiManager()
{
}

bool voip_proxy::VoipEmojiManager::addMap(const unsigned _sw, const unsigned _sh, const std::string& _path, const std::vector<unsigned>& _codePoints, const size_t _size)
{
    im_assert(!_codePoints.empty());

    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    const size_t id = std::hash<std::string>()(_path);
    bool existsSameSize = false;
    bool exactlySame    = false;

    if (it != codeMaps_.end())
    {
        existsSameSize = (it->codePointSize == _size);
        exactlySame    = existsSameSize && (it->id == id);
    }

    if (exactlySame)
        return true;

    if (existsSameSize)
        codeMaps_.erase(it);

    CodeMap codeMap;
    codeMap.id            = id;
    codeMap.path          = _path;
    codeMap.codePointSize = _size;
    codeMap.sw            = _sw;
    codeMap.sh            = _sh;
    codeMap.codePoints    = _codePoints;

    codeMaps_.push_back(codeMap);
    std::sort(
        codeMaps_.begin(), codeMaps_.end(), [] (const CodeMap& l, const CodeMap& r)
        {
            return l.codePointSize < r.codePointSize;
        }
    );
    return true;
}

bool voip_proxy::VoipEmojiManager::getEmoji(const unsigned _codePoint, const size_t _size, QImage& _image)
{
    std::vector<CodeMap>::const_iterator it = std::lower_bound(codeMaps_.begin(), codeMaps_.end(), _size, [] (const CodeMap& l, const size_t& r)
    {
        return l.codePointSize < r;
    });

    if (it == codeMaps_.end())
    {
        if (codeMaps_.empty())
            return false;
        it = codeMaps_.end();

        im_assert(false);
        return false;
    }

    const CodeMap& codeMap = *it;

    if (codeMap.id != activeMapId_)
    {   // reload code map
        activeMap_.load(QString::fromStdString(codeMap.path));
        activeMapId_ = codeMap.id;
    }

    const size_t mapW = activeMap_.width();
    const size_t mapH = activeMap_.height();
    im_assert(mapW != 0 && mapH != 0);

    if (mapW == 0 || mapH == 0)
        return false;

    const size_t rows = mapH / codeMap.sh;
    const size_t cols = mapW / codeMap.sw;

    for (size_t ix = 0; ix < codeMap.codePoints.size(); ++ix)
    {
        const unsigned code = codeMap.codePoints[ix];
        if (code == _codePoint)
        {
            const size_t targetRow = ix / cols;
            const size_t targetCol = ix - (targetRow * cols);

            if (targetRow >= rows)
                return false;

            const QRect r(targetCol * codeMap.sw, targetRow * codeMap.sh, codeMap.sw, codeMap.sh);
            _image = activeMap_.copy(r);
            return true;
        }
    }
    im_assert(false);
    return false;
}
