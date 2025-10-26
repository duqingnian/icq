#include "stdafx.h"
#include "UrlCommand.h"
#include "utils/InterConnector.h"
#include "main_window/contact_list/ContactListModel.h"
#include "cache/stickers/stickers.h"
#include "utils/utils.h"
#include "utils/UrlParser.h"
#include "../previewer/toast.h"
#include "app_config.h"
#include "url_config.h"
#include "features.h"
#include "../../common.shared/config/config.h"
#include "../main_window/AppsPage.h"

#include "../main_window/mini_apps/MiniAppsContainer.h"

namespace
{
    bool checkJoinQuery(const QUrl& _url)
    {
        const auto items = QUrlQuery(_url).queryItems();
        const bool join = std::any_of(items.cbegin(), items.cend(), [](const auto& param)
        {
            return param.first == u"join" && param.second == u'1';
        });

        return join;
    }

    bool isMiniAppHost(QStringView _host)
    {
        return (_host.compare(url_command_open_app(), Qt::CaseInsensitive) == 0) || (_host.compare(url_command_open_miniapp(), Qt::CaseInsensitive) == 0);
    }
}


std::unique_ptr<UrlCommand> UrlCommandBuilder::makeCommand(QString _url, Context _context)
{
    if (_url.endsWith(u'/'))
        _url.chop(1);

    const static auto schemesWithSlashes = []() {
        const auto& urlSchemes = Utils::urlSchemes();
        std::vector<QString> schemes;
        schemes.reserve(urlSchemes.size());
        for (const auto& x : urlSchemes)
            schemes.push_back(x % u"://");
        return schemes;
    }();

    QString currentScheme;
    if (const auto it = std::find_if(schemesWithSlashes.begin(), schemesWithSlashes.end(), [&_url](const auto &x) { return _url.startsWith(x, Qt::CaseInsensitive); }); it != schemesWithSlashes.end())
        currentScheme = *it;

    const auto slashCount = currentScheme.count(u'/');
    const auto noAdditionalSlashes = _url.count(u'/') == slashCount; // redundant, but safe

    if (!currentScheme.isEmpty()) // [icq|magent|myteam-messenger|itd-messenger]://uin
    {
        constexpr auto startMarker = QStringView(u"?start=");
        auto cmd = QStringView(_url).mid(currentScheme.size());
        const auto startIdx = cmd.indexOf(startMarker);

        if (_url.contains(u"attach_phone_result"))
        {
            return std::make_unique<PhoneAttachedCommand>(_url.contains(u"success=1"), _url.contains(u"cancel=1"));
        }
        else if ((noAdditionalSlashes || startIdx != -1) && (!cmd.contains(u"copy_to_buffer")))
        {
            QStringView params;
            if (startIdx != -1)
            {
                params = cmd.mid(startIdx + startMarker.size());
                cmd = cmd.left(startIdx);
                if (cmd.endsWith(u'/'))
                    cmd.chop(1);
            }

            return std::make_unique<OpenIdCommand>(cmd.toString(), _context, startIdx != -1 ? std::make_optional(params.toString()) : std::nullopt);
        }
    }

    constexpr QStringView wwwDot = u"www.";

    const QUrl url(_url);
    const QString scheme = url.scheme();
    QStringView hostView(url.host());
    QString path = url.path();

    if (hostView.startsWith(wwwDot))
        hostView = hostView.mid(wwwDot.size());

    if (!path.isEmpty())
        path.remove(0, 1);

    if (path.endsWith(u'/'))
        path.chop(1);

    if (Utils::isInternalScheme(scheme)) // check for urls starting with icq://, magent://, myteam-messenger://, itd-messenger://
    {
        if (hostView.compare(url_command_open_app(), Qt::CaseInsensitive) == 0 && path.compare(url_command_create_group(), Qt::CaseInsensitive) == 0)  // [icq|myteam-messenger]://app/newchat
            return std::make_unique<CreateChatCommand>(_url);

        else if (hostView.compare(url_command_open_app(), Qt::CaseInsensitive) == 0 && path.compare(url_command_create_channel(), Qt::CaseInsensitive) == 0) // [icq|myteam-messenger]://app/newchannel
            return std::make_unique<CreateChannelCommand>(_url);

        else if (hostView.compare(url_command_join_livechat(), Qt::CaseInsensitive) == 0)            // [icq|magent|myteam-messenger|itd-messenger]://chat/stamp[?join=1]
            return std::make_unique<OpenChatCommand>(path, checkJoinQuery(url));

        else if (hostView.compare(url_command_open_profile(), Qt::CaseInsensitive) == 0)        // [icq|magent|myteam-messenger|itd-messenger]://people/uin
            return std::make_unique<OpenContactCommand>(path);

        else if (isMiniAppHost(hostView))                                                       // [icq|magent|myteam-messenger|itd-messenger]://miniapp/calendar ://app/calendar
            return std::make_unique<OpenAppCommand>(path, url.query(), url.fragment());

        else if (hostView.compare(url_command_stickerpack_info(), Qt::CaseInsensitive) == 0)    // [icq|magent|myteam-messenger|itd-messenger]://s/store_id
            return std::make_unique<ShowStickerpackCommand>(path, _context);

        else if (hostView.compare(url_command_service(), Qt::CaseInsensitive) == 0)             // [icq|magent|myteam-messenger|itd-messenger]://service/*
            return std::make_unique<ServiceCommand>(_url);

        else if (hostView.compare(url_command_vcs_call(), Qt::CaseInsensitive) == 0)            // [client]://call/*
            return std::make_unique<VCSCommand>(_url);

        else if (hostView.compare(url_command_open_auth_modal_response(), Qt::CaseInsensitive) == 0)  // [client]://authmodal
            return std::make_unique<AuthModalResult>(path, url.query());

        else if (hostView.compare(url_command_open_both_profile_and_chat(), Qt::CaseInsensitive) == 0)// [icq|magent|myteam-messenger|itd-messenger]://profile/[uin|stamp]
            return std::make_unique<OpenIdCommand>(path);

        else if (hostView.compare(url_command_copy_to_clipboard(), Qt::CaseInsensitive) == 0)// [icq|magent|myteam-messenger|itd-messenger]://copy_to_buffer?text=текст_для_копирования
            return std::make_unique<CopyToClipboardCommand>(url.query().remove(qsl("text="), Qt::CaseInsensitive));
    }
    else
    {
        constexpr auto icqCom = u"icq.com";
        const auto& stickerStore = Ui::getUrlConfig().getUrlStickerShare();
        static const auto agentProfile = Features::getProfileDomainAgent();
        static const auto profile = Features::getProfileDomain();

        const auto parts = path.splitRef(u'/');

        if (config::get().is_on(config::features::open_icqcom_link) && hostView.compare(icqCom, Qt::CaseInsensitive) == 0)
        {
            const QRegularExpression re(qsl("^\\d*$")); // only numbers
            const auto match = re.match(path);

            Utils::UrlParser p;
            p.process(path);
            const auto isEmail = p.hasUrl() && p.isEmail();

            if (!path.contains(u'/') && (match.hasMatch() || isEmail))                     // icq.com/uin, where uin is either an email or a string of numbers
                return std::make_unique<OpenContactCommand>(path);
            else if (parts.size() > 1 && QStringView(parts.first()).compare(url_command_join_livechat(), Qt::CaseInsensitive) == 0)  // icq.com/chat/stamp[?join=1]
                return std::make_unique<OpenChatCommand>(parts[1].toString(), checkJoinQuery(url));
        }
        else if (_url.contains(agentProfile)) // for unparcable by QUrl hosts, containing '/profile'
        {
            const auto profileParts = _url.splitRef(QStringView(url_commandpath_agent_profile()) % u'/');
            if (profileParts.size() > 1)
                return std::make_unique<OpenIdCommand>(profileParts[1].toString());
        }
        else if (_url.contains(stickerStore)) // cicq.org/s/store_id
        {
            if (parts.size() > 1 && QStringView(parts.first()).compare(url_command_stickerpack_info(), Qt::CaseInsensitive) == 0)
                return std::make_unique<ShowStickerpackCommand>(parts[1].toString(), _context);
        }
        else if (hostView.compare(profile, Qt::CaseInsensitive) == 0)
        {
            if (!path.contains(u'/')) // icq.com/uin
                return std::make_unique<OpenIdCommand>(std::move(path), _context);
        }
        else if (Utils::isUrlVCS(_url))
        {
            return std::make_unique<VCSCommand>(_url);
        }
    }

    return std::make_unique<UrlCommand>();
}

//////////////////////////////////////////////////////////////////////////
// OpenContactCommand
//////////////////////////////////////////////////////////////////////////

OpenContactCommand::OpenContactCommand(const QString& _aimId)
    : aimId_(_aimId)
{
    isValid_ = !aimId_.isEmpty();
}

void OpenContactCommand::execute()
{
    Utils::openDialogOrProfile(aimId_);
}

//////////////////////////////////////////////////////////////////////////
// OpenAppCommand
//////////////////////////////////////////////////////////////////////////

OpenAppCommand::OpenAppCommand(const QString& _app, const QString& _urlQuery, const QString& _urlFragment)
    : app_(_app)
    , urlQuery_(_urlQuery)
    , urlFragment_(_urlFragment)
{
    isValid_ = !app_.isEmpty();
}

void OpenAppCommand::execute()
{
    if (Logic::GetAppsContainer()->isAppEnabled(app_))
        Q_EMIT Utils::InterConnector::instance().requestApp(app_, urlQuery_, urlFragment_);
}

//////////////////////////////////////////////////////////////////////////
// OpenChatCommand
//////////////////////////////////////////////////////////////////////////

OpenChatCommand::OpenChatCommand(const QString &_stamp, bool _join)
    : stamp_(_stamp)
    , join_(_join)
{
    isValid_ = !stamp_.isEmpty();
}

void OpenChatCommand::execute()
{
    Logic::getContactListModel()->joinLiveChat(stamp_, join_);
}

//////////////////////////////////////////////////////////////////////////
// OpenIdCommand
//////////////////////////////////////////////////////////////////////////

OpenIdCommand::OpenIdCommand(QString _id, UrlCommandBuilder::Context _context, std::optional<QString> _botParams)
    : id_(std::move(_id))
    , context_(_context)
    , params_(std::move(_botParams))
{
    isValid_ = !id_.isEmpty();
}

void OpenIdCommand::execute()
{
    Q_EMIT Utils::InterConnector::instance().openDialogOrProfileById(id_, context_ == UrlCommandBuilder::Context::External, params_);
}

//////////////////////////////////////////////////////////////////////////
// PhoneAttachedCommand
//////////////////////////////////////////////////////////////////////////

PhoneAttachedCommand::PhoneAttachedCommand(const bool _success, const bool _cancel)
    : success_(_success)
    , cancel_(_cancel)
{
    isValid_ = true;
}

void PhoneAttachedCommand::execute()
{
    if (cancel_)
        Q_EMIT Utils::InterConnector::instance().phoneAttachmentCancelled();
//     else
//         Q_EMIT Utils::InterConnector::instance().phoneAttached(success_);
}

//////////////////////////////////////////////////////////////////////////
// ShowStickerpackCommand
//////////////////////////////////////////////////////////////////////////

ShowStickerpackCommand::ShowStickerpackCommand(const QString &_storeId, UrlCommandBuilder::Context _context)
    : storeId_(_storeId)
    , context_(_context)
{
    isValid_ = !storeId_.isEmpty();
}

void ShowStickerpackCommand::execute()
{
    Ui::Stickers::StatContext context;

    if (context_ == UrlCommandBuilder::Context::External)
        context = Ui::Stickers::StatContext::Browser;
    else
        context = Ui::Stickers::StatContext::Chat;

    Ui::Stickers::showStickersPackByStoreId(storeId_, context);
}

//////////////////////////////////////////////////////////////////////////
// ServiceCommand
//////////////////////////////////////////////////////////////////////////

ServiceCommand::ServiceCommand(const QString &_url)
    : url_(_url)
{
    isValid_ = !url_.isEmpty();
}

void ServiceCommand::execute()
{
    QTimer::singleShot(0, [url = url_]()
    {
        Q_EMIT Utils::InterConnector::instance().gotServiceUrls({url});
    });
}

//////////////////////////////////////////////////////////////////////////
// VCSCommand
//////////////////////////////////////////////////////////////////////////

VCSCommand::VCSCommand(const QString &_url)
    : url_(_url)
{
    isValid_ = !url_.isEmpty();
}

void VCSCommand::execute()
{
    if (!isValid_)
        return;

    QTimer::singleShot(0, [url = url_]()
    {
        Q_EMIT Utils::InterConnector::instance().closeAnySemitransparentWindow({ Utils::CloseWindowInfo::Initiator::Unknown, Utils::CloseWindowInfo::Reason::Keep_Sidebar });

        auto confirm = Utils::GetConfirmationWithTwoButtons(
            QT_TRANSLATE_NOOP("popup_window", "Cancel"),
            QT_TRANSLATE_NOOP("popup_window", "Yes"),
            QT_TRANSLATE_NOOP("popup_window", "Do you want to join conference?"),
            QT_TRANSLATE_NOOP("popup_window", "Join confirmation"),
            nullptr
        );
        if (!confirm)
            return;

#ifndef STRIP_VOIP
        Ui::GetDispatcher()->getVoipController().setStartVCS(url);
#endif
        return;
    });
}

//////////////////////////////////////////////////////////////////////////
// AuthModalResult
//////////////////////////////////////////////////////////////////////////

AuthModalResult::AuthModalResult(const QString& _requestId, const QString& _urlQuery)
    : urlQuery_ { _urlQuery }
    , requestId_ { _requestId }
{
    isValid_ = !urlQuery_.isEmpty();
}

void AuthModalResult::execute()
{
    Q_EMIT Utils::InterConnector::instance().onOpenAuthModalResponse(requestId_, urlQuery_);
}

//////////////////////////////////////////////////////////////////////////
// CreateChatCommand
//////////////////////////////////////////////////////////////////////////

CreateChatCommand::CreateChatCommand(const QString& _url)
{
    isValid_ = !_url.isEmpty();
}

void CreateChatCommand::execute()
{
    Q_EMIT Utils::InterConnector::instance().createGroupChat();
}

//////////////////////////////////////////////////////////////////////////
// CreateChannelCommand
//////////////////////////////////////////////////////////////////////////

CreateChannelCommand::CreateChannelCommand(const QString& _url)
{
    isValid_ = !_url.isEmpty();
}

void CreateChannelCommand::execute()
{
    Q_EMIT Utils::InterConnector::instance().createChannel();
}

//////////////////////////////////////////////////////////////////////////
// CopyToClipboardCommand
//////////////////////////////////////////////////////////////////////////

CopyToClipboardCommand::CopyToClipboardCommand(const QString& _text)
: text_(_text)
{
    isValid_ = !_text.isEmpty();
}

void CopyToClipboardCommand::execute()
{
    QApplication::clipboard()->setText(text_);
    Utils::showCopiedToast();
}
