#include "stdafx.h"

#include "../../../../corelib/enumerations.h"
#include "../../../../common.shared/loader_errors.h"
#include "../../../../gui.shared/constants.h"

#include "../../../core_dispatcher.h"
#include "../../../gui_settings.h"
#include "../../../utils/log/log.h"
#include "../../../utils/utils.h"
#include "../../../utils/features.h"
#include "previewer/toast.h"
#include "utils/medialoader.h"

#include "../FileSizeFormatter.h"
#include "../MessageStyle.h"
#include "../HistoryControlPage.h"
#include "main_window/history_control/FileStatus.h"
#include "../../mediatype.h"
#include "../../contact_list/ContactListModel.h"
#include "../../containers/FileSharingMetaContainer.h"

#include "IFileSharingBlockLayout.h"
#include "FileSharingUtils.h"

#include "FileSharingBlockBase.h"
#include "ComplexMessageItem.h"
#include "IItemBlock.h"

namespace
{
    constexpr std::chrono::milliseconds getDataTransferTimeout() noexcept
    {
        return std::chrono::milliseconds(300);
    }
}

UI_COMPLEX_MESSAGE_NS_BEGIN

FileSharingBlockBase::FileSharingBlockBase(
    ComplexMessageItem *_parent,
    const QString& _link,
    const core::file_sharing_content_type& _type)
    : GenericBlock(
        _parent,
        _link,
        MenuFlags::MenuFlagFileCopyable,
        true)
    , Link_(Utils::normalizeLink(_link).toString())
    , fileId_(extractIdFromFileSharingUri(getLink()))
    , Type_(std::make_unique<core::file_sharing_content_type>(_type))
{
    Testing::setAccessibleName(this, u"AS HistoryPage messageFileSharing " % QString::number(_parent->getId()));
}

FileSharingBlockBase::~FileSharingBlockBase() = default;

void FileSharingBlockBase::clearSelection()
{
    if (IsSelected_)
        setSelected(false);
}

QString FileSharingBlockBase::formatRecentsText() const
{
    using namespace core;

    const auto& type = getType();
    if (type.is_sticker())
        return QT_TRANSLATE_NOOP("contact_list", "Sticker");

    switch (type.type_)
    {
        case file_sharing_base_content_type::gif:
            return QT_TRANSLATE_NOOP("contact_list", "GIF");

        case file_sharing_base_content_type::image:
            return QT_TRANSLATE_NOOP("contact_list", "Photo");

        case file_sharing_base_content_type::video:
            return QT_TRANSLATE_NOOP("contact_list", "Video");

        case file_sharing_base_content_type::ptt:
            return QT_TRANSLATE_NOOP("contact_list", "Voice message");

        default:
            ;
    }

    return QT_TRANSLATE_NOOP("contact_list", "File");
}

Ui::MediaType FileSharingBlockBase::getMediaType() const
{
    using namespace core;

    const auto& type = getType();
    if (type.is_sticker())
        return Ui::MediaType::mediaTypeSticker;

    switch (type.type_)
    {
    case file_sharing_base_content_type::gif:
        return Ui::MediaType::mediaTypeFsGif;

    case file_sharing_base_content_type::image:
        return Ui::MediaType::mediaTypeFsPhoto;

    case file_sharing_base_content_type::video:
        return Ui::MediaType::mediaTypeFsVideo;

    case file_sharing_base_content_type::ptt:
        return Ui::MediaType::mediaTypePtt;

    default:
        ;
    }

    return Ui::MediaType::mediaTypeFileSharing;
}

IItemBlockLayout* FileSharingBlockBase::getBlockLayout() const
{
    im_assert(Layout_);
    return Layout_;
}

QString FileSharingBlockBase::getProgressText() const
{
    im_assert(Meta_.size_ >= -1);
    im_assert(BytesTransferred_ >= -1);

    const auto isInvalidProgressData = ((Meta_.size_ <= -1) || (BytesTransferred_ < -1));
    if (isInvalidProgressData)
    {
        return QString();
    }

    return ::HistoryControl::formatProgressText(Meta_.size_, BytesTransferred_);
}

Data::FString FileSharingBlockBase::getSourceText() const
{
    if (Link_.isEmpty())
        return Data::FString(Link_);

    const auto mediaType = getPlaceholderFormatText();
    const auto id = getFileSharingId();
    const auto idText = isBlockedFileSharing() ? filesharingPlaceholder().toString() : id.fileId_;
    const auto source = (isBlockedFileSharing() || !id.sourceId_) ? QString() : (u"?source=" % *id.sourceId_);
    return QString(u'[' % mediaType % u": " % idText % source % u']');
}

Data::FString FileSharingBlockBase::getTextInstantEdit() const
{
    return Data::FString(Link_);
}

QString FileSharingBlockBase::getPlaceholderText() const
{
    return u'[' % QT_TRANSLATE_NOOP("placeholders", "Failed to download file or media") % u']';
}

Data::FString FileSharingBlockBase::getSelectedText(const bool, const TextDestination) const
{
    if (IsSelected_)
        return getSourceText();

    return {};
}

void FileSharingBlockBase::initialize()
{
    connectSignals();

    initializeFileSharingBlock();

    inited_ = true;
}

bool FileSharingBlockBase::isBubbleRequired() const
{
    return isStandalone() && (!isPreviewable() || isSmallPreview());
}

bool FileSharingBlockBase::isMarginRequired() const
{
    return !isStandalone() || !isPreviewable();
}

bool FileSharingBlockBase::isSelected() const
{
    return IsSelected_;
}

const QString& FileSharingBlockBase::getFileLocalPath() const
{
    return Meta_.localPath_;
}

int64_t FileSharingBlockBase::getLastModified() const
{
    return Meta_.lastModified_;
}

const QString& FileSharingBlockBase::getFilename() const
{
    return Meta_.filenameShort_;
}

IFileSharingBlockLayout* FileSharingBlockBase::getFileSharingLayout() const
{
    im_assert(Layout_);
    return Layout_;
}

const Utils::FileSharingId& FileSharingBlockBase::getFileSharingId() const
{
    return fileId_;
}

int64_t FileSharingBlockBase::getFileSize() const
{
    im_assert(Meta_.size_ >= -1);

    return Meta_.size_;
}

int64_t FileSharingBlockBase::getBytesTransferred() const
{
    return BytesTransferred_;
}

const QString& FileSharingBlockBase::getLink() const
{
    return Link_;
}

const core::file_sharing_content_type& FileSharingBlockBase::getType() const
{
    return *Type_;
}

const QString& FileSharingBlockBase::getDirectUri() const
{
    return Meta_.downloadUri_;
}

bool FileSharingBlockBase::isFileDownloaded() const
{
    return !Meta_.localPath_.isEmpty();
}

bool FileSharingBlockBase::isFileDownloading() const
{
    im_assert(DownloadRequestId_ >= -1);

    return DownloadRequestId_ > 0;
}

bool FileSharingBlockBase::isFileUploading() const
{
    return !uploadId_.isEmpty();
}

bool FileSharingBlockBase::isGifImage() const
{
    return getType().is_gif();
}

bool FileSharingBlockBase::isLottie() const
{
    return getType().is_lottie();
}

bool FileSharingBlockBase::isImage() const
{
    return getType().is_image();
}

bool FileSharingBlockBase::isPtt() const
{
    return  getType().is_ptt();
}

bool FileSharingBlockBase::isVideo() const
{
    return getType().is_video();
}

bool FileSharingBlockBase::isVirusInfected() const
{
    return status_.isVirusInfectedType();
}

bool FileSharingBlockBase::isAllowedByAntivirus() const
{
    return status_.isAntivirusAllowedType();
}

core::antivirus::check FileSharingBlockBase::antivirusCheckState() const
{
    return Meta_.antivirusCheck_;
}

Data::FilesPlaceholderMap FileSharingBlockBase::getFilePlaceholders()
{
    Data::FilesPlaceholderMap files;
    if (const auto& link = getLink(); !link.isEmpty())
        files.insert({ link, getSourceText().string() });
    return files;
}

bool FileSharingBlockBase::isDownloadAllowed() const
{
    return !isPlainFile() || isFileTrustAllowed() && isAllowedByAntivirus();
}

bool FileSharingBlockBase::isBlockedFileSharing() const
{
    return status_.isTrustRequired();
}

bool FileSharingBlockBase::isFileSharingWithStatus() const
{
    return needShowStatus(status_);
}

bool FileSharingBlockBase::isFileTrustAllowed() const
{
    return !isBlockedFileSharing() || MyInfo()->isTrusted();
}

bool FileSharingBlockBase::isDraggable() const
{
    if (isPlainFile())
        return !Logic::getContactListModel()->isTrustRequired(getChatAimid());

    return GenericBlock::isDraggable();
}

bool FileSharingBlockBase::isPlainFile() const
{
    return !isPreviewable();
}

bool FileSharingBlockBase::isPreviewable() const
{
    return (isImage() || isPlayableType()) && !isVirusInfected();
}

bool FileSharingBlockBase::isPlayable() const
{
    return isPlayableType() && !isVirusInfected();
}

bool FileSharingBlockBase::isPlayableType() const
{
    return isVideo() || isGifImage() || isLottie();
}

void FileSharingBlockBase::onMenuCopyLink()
{
    QApplication::clipboard()->setText(getLink());
}

void FileSharingBlockBase::onMenuOpenInBrowser()
{
    QDesktopServices::openUrl(getLink());
}

void FileSharingBlockBase::showFileInDir(const Utils::OpenAt _inFolder)
{
    if (const QFileInfo f(getFileLocalPath()); !f.exists() || f.size() != getFileSize() || f.lastModified().toTime_t() != getLastModified())
    {
        if (isPlainFile())
            return startDownloading(true, Priority::High);
        else
            return onMenuSaveFileAs();
    }

    Utils::openFileOrFolder(getChatAimid(), getFileLocalPath(), _inFolder);
}

void FileSharingBlockBase::onMenuCopyFile()
{
    if (QFile::exists(getFileLocalPath()))
    {
        Utils::copyFileToClipboard(getFileLocalPath());
        Utils::showCopiedToast();
    }
    else if (isBlockedFileSharing() && !MyInfo()->isTrusted())
    {
        showFileStatusToast(status_.type());
    }
    else
    {
        CopyFile_ = true;

        if (isFileDownloading())
            return;

        startDownloading(true);
    }
}

void FileSharingBlockBase::onMenuSaveFileAs()
{
    if (Meta_.filenameShort_.isEmpty())
    {
        im_assert(!"no filename");
        return;
    }

    QUrl urlParser(Meta_.filenameShort_);

    Utils::saveAs(urlParser.fileName(), [weak_this = QPointer(this)](const QString& file, const QString& dir_result, bool _exportAsPng)
    {
        if (!weak_this)
            return;

        const auto addTrailingSlash = (!dir_result.endsWith(u'\\') && !dir_result.endsWith(u'/'));
        const auto slash = addTrailingSlash ? QStringView(u"/") : QStringView();
        const QString dir = dir_result % slash % file;

        auto saver = new Utils::FileSaver(weak_this);
        saver->save([dir](bool _success, const QString& /*_savedPath*/)
        {
            if (_success)
                showFileSavedToast(dir);
            else
                showErrorToast();
        }, weak_this->getLink(), dir, _exportAsPng);
    });
}

void FileSharingBlockBase::requestMetainfo(bool _isPreview)
{
    if (loadedFromLocal_)
        return;

    const auto& fileId = getFileSharingId().fileId_;

    if (fileId == filesharingPlaceholder())
    {
        if (!Features::isRestrictedFilesEnabled())
            getParentComplexMessage()->replaceBlockWithSourceText(this, ReplaceReason::NoMeta);
        return;
    }

    if (_isPreview)
    {
        auto origMaxSize = -1;
        if (!fileId.isEmpty())
        {
            if (const auto originalSize = extractSizeFromFileSharingId(fileId); !originalSize.isEmpty())
                origMaxSize = Utils::scale_bitmap(std::max(originalSize.width(), originalSize.height()));
        }

        const auto requestId = GetDispatcher()->getFileSharingPreviewSize(getLink(), origMaxSize);
        im_assert(requestId > 0);
        im_assert(PreviewMetaRequestId_ == -1);
        PreviewMetaRequestId_ = requestId;

        __TRACE(
            "prefetch",
            "initiated file sharing preview metadata downloading\n"
            "    contact=<" << getSenderAimid() << ">\n"
            "    fsid=<" << getFileSharingId().fileId_ << ">\n"
            "    request_id=<" << requestId << ">");
    }
    else
    {
        Logic::GetFileSharingMetaContainer()->requestFileSharingMetaInfo(getFileSharingId(), this);
    }

}

void FileSharingBlockBase::setBlockLayout(IFileSharingBlockLayout* _layout)
{
    im_assert(_layout);

    if (Layout_)
        delete Layout_;

    Layout_ = _layout;
}

void FileSharingBlockBase::setSelected(const bool _isSelected)
{
    if (IsSelected_ == _isSelected)
        return;

    IsSelected_ = _isSelected;

    update();
}

void FileSharingBlockBase::startDownloading(const bool _sendStats, Priority _priority)
{
    if (!isDownloadAllowed())
        return;

    im_assert(!isFileDownloading());

    auto downloading = [guard = QPointer(this), _sendStats, _priority]()
    {
        if (!guard)
            return;

        if (_sendStats)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_download_file);

        guard->DownloadRequestId_ = GetDispatcher()->downloadSharedFile(
            guard->getChatAimid(),
            guard->getLink(),
            false,
            guard->SaveAs_, //QString(), // don't loose user's 'save as' path
            Priority::High == _priority ? true : false);
    };

    if (!Logic::GetFileSharingMetaContainer()->isDownloading(getFileSharingId()) && Meta_.size_ != -1  && !Meta_.downloadUri_.isEmpty())
    {
        downloading();
        delayedCallDownloading_ = {};
    }
    else
    {
        delayedCallDownloading_ = std::move(downloading);
        if ((!isFileSharingWithStatus() || (isBlockedFileSharing() && MyInfo()->isTrusted())) && Meta_.downloadUri_.isEmpty())
            requestMetainfo(false);
    }

    startDataTransferTimeoutTimer();

    onDataTransferStarted();
}

void FileSharingBlockBase::stopDataTransfer()
{
    if (isFileDownloading())
    {
        GetDispatcher()->abortSharedFileDownloading(Link_, DownloadRequestId_);
    }
    else if (isFileUploading())
    {
        GetDispatcher()->abortSharedFileUploading(getChatAimid(), Meta_.localPath_, uploadId_);
        Q_EMIT removeMe();
    }

    stopDataTransferTimeoutTimer();

    DownloadRequestId_ = -1;

    uploadId_.clear();

    BytesTransferred_ = -1;

    delayedCallDownloading_ = {};

    onDataTransferStopped();
}

int64_t FileSharingBlockBase::getPreviewMetaRequestId() const
{
    return PreviewMetaRequestId_;
}

void FileSharingBlockBase::clearPreviewMetaRequestId()
{
    PreviewMetaRequestId_ = -1;
}

void FileSharingBlockBase::cancelRequests()
{
    if (!GetDispatcher() || !inited_)
        return;

    if (isFileDownloading())
    {
        GetDispatcher()->abortSharedFileDownloading(Link_, DownloadRequestId_);
        GetDispatcher()->cancelLoaderTask(Link_, DownloadRequestId_);
    }
}

void FileSharingBlockBase::setFileName(const QString& _filename)
{
    Meta_.filenameShort_ = _filename;
}

void FileSharingBlockBase::setFileSize(int64_t _size)
{
    Meta_.size_ = _size;
}

void FileSharingBlockBase::setLocalPath(const QString& _localPath)
{
    Meta_.localPath_ = _localPath;
    const QFileInfo f(_localPath);
    Meta_.size_ = f.size();
    Meta_.lastModified_ = f.lastModified().toTime_t();
}

void FileSharingBlockBase::setLoadedFromLocal(bool _value)
{
    loadedFromLocal_ = _value;
}

void FileSharingBlockBase::setUploadId(const QString& _id)
{
    uploadId_ = _id;
}

bool FileSharingBlockBase::isInited() const
{
    return inited_;
}

QStringList FileSharingBlockBase::messageLinks() const
{
    QStringList links;
    links += getLink();
    return links;
}

void FileSharingBlockBase::connectSignals()
{
    if (!isInited())
    {
        const auto& type = getType();
        if (!type.is_sticker() || type.is_gif())
        {
            connect(GetDispatcher(), &core_dispatcher::fileSharingFileDownloaded, this, &FileSharingBlockBase::onFileDownloaded);
            connect(GetDispatcher(), &core_dispatcher::fileSharingFileDownloading, this, &FileSharingBlockBase::onFileDownloading);
            connect(GetDispatcher(), &core_dispatcher::fileSharingError, this, &FileSharingBlockBase::onFileSharingError);
            connect(GetDispatcher(), &core_dispatcher::fileSharingUploadingResult, this, &FileSharingBlockBase::fileSharingUploadingResult);
            connect(GetDispatcher(), &core_dispatcher::fileSharingUploadingProgress, this, &FileSharingBlockBase::fileSharingUploadingProgress);

            if (isPreviewable())
                connect(GetDispatcher(), &core_dispatcher::fileSharingPreviewMetainfoDownloaded, this, &FileSharingBlockBase::onPreviewMetainfoDownloadedSlot);
        }
    }
}

bool FileSharingBlockBase::isDownload(const Data::FileSharingDownloadResult& _result) const
{
    return !_result.requestedUrl_.isEmpty();
}

QString FileSharingBlockBase::getPlaceholderFormatText() const
{
    using namespace core;

    const auto& primalType = getType();

    if( primalType.is_sticker())
        return qsl("Sticker");

    switch (primalType.type_)
    {
        case file_sharing_base_content_type::gif:
            return qsl("GIF");

        case file_sharing_base_content_type::image:
            return qsl("Photo");

        case file_sharing_base_content_type::video:
            return qsl("Video");

        case file_sharing_base_content_type::ptt:
            return qsl("Voice");

        default:
        {
            if (const auto &filename = getFilename(); !filename.isEmpty())
                return FileSharing::getPlaceholderForType(FileSharing::getFSType(filename));
        }
    }
    return qsl("File");
}

void FileSharingBlockBase::startDataTransferTimeoutTimer()
{
    if (!dataTransferTimeout_)
    {
        dataTransferTimeout_ = new QTimer(this);
        dataTransferTimeout_->setSingleShot(true);
        dataTransferTimeout_->setInterval(getDataTransferTimeout());

        connect(dataTransferTimeout_, &QTimer::timeout, this, [this]()
        {
            // emulate 1% of transferred data for the spinner
            onDataTransfer(1, 100, false);
        });
    }

    dataTransferTimeout_->start();
}

void FileSharingBlockBase::stopDataTransferTimeoutTimer()
{
    if (dataTransferTimeout_)
        dataTransferTimeout_->stop();
}

void FileSharingBlockBase::onFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result)
{
    if (_seq != DownloadRequestId_ && getLink() != _result.requestedUrl_)
        return;

    im_assert(!_result.filename_.isEmpty());

    DownloadRequestId_ = -1;

    BytesTransferred_ = -1;

    SaveAs_.clear();

    setLocalPath(_result.filename_);

    Meta_.savedByUser_ = _result.savedByUser_;

    onDownloaded();

    if (CopyFile_)
    {
        CopyFile_ = false;
        Utils::copyFileToClipboard(_result.filename_);
        Utils::showCopiedToast();

        return;
    }

    onDownloadedAction();

    if (isDownload(_result) && isPlainFile() && !isPtt())
        Utils::showDownloadToast(getChatAimid(), _result);
}

void FileSharingBlockBase::onFileDownloading(qint64 _seq, QString /*_rawUri*/, qint64 _bytesTransferred, qint64 _bytesTotal)
{
    im_assert(_bytesTotal > 0);

    if (_seq != DownloadRequestId_)
        return;

    stopDataTransferTimeoutTimer();

    BytesTransferred_ = _bytesTransferred;

    onDataTransfer(_bytesTransferred, _bytesTotal);
}

void FileSharingBlockBase::processMetaInfo(const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta)
{
    im_assert(_fsId == getFileSharingId());
    if (_fsId != getFileSharingId())
        return;

    Meta_.mergeWith(_meta);

    onMetainfoDownloaded();

    notifyBlockContentsChanged();

    if (delayedCallDownloading_)
        delayedCallDownloading_();
}

void FileSharingBlockBase::onFileSharingError(qint64 _seq, QString _rawUri, qint32 _errorCode)
{
    im_assert(_seq > 0);

    const auto isDownloadRequestFailed = (DownloadRequestId_ == _seq);
    if (isDownloadRequestFailed)
    {
        DownloadRequestId_ = -1;

        if (!SaveAs_.isEmpty())
        {
            showErrorToast();
            SaveAs_.clear();
        }

        onDownloadingFailed(_seq);
    }

    const auto isPreviewMetainfoRequestFailed = (PreviewMetaRequestId_ == _seq);
    if (isPreviewMetainfoRequestFailed)
        clearPreviewMetaRequestId();

    if (static_cast<loader_errors>(_errorCode) == loader_errors::move_file)
        onDownloaded();//stop animation
}

void FileSharingBlockBase::processMetaInfoError(const Utils::FileSharingId& _fsId, qint32 _errorCode)
{
    im_assert(_fsId == getFileSharingId());
    if (Q_UNLIKELY(_fsId != getFileSharingId()))
        return;

    clearPreviewMetaRequestId();
    getParentComplexMessage()->replaceBlockWithSourceText(this, ReplaceReason::NoMeta);


    if (static_cast<loader_errors>(_errorCode) == loader_errors::move_file)
        onDownloaded();//stop animation
}

void FileSharingBlockBase::onPreviewMetainfoDownloadedSlot(qint64 _seq, QString _miniPreviewUri, QString _fullPreviewUri)
{
    if (_seq != PreviewMetaRequestId_)
        return;

    im_assert(isPreviewable());

    clearPreviewMetaRequestId();

    Meta_.miniPreviewUri_ = _miniPreviewUri;
    Meta_.fullPreviewUri_ = _fullPreviewUri;

    onPreviewMetainfoDownloaded();
}

void FileSharingBlockBase::fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred)
{
    if (_uploadingId != uploadId_)
        return;

    stopDataTransferTimeoutTimer();

    BytesTransferred_ = _bytesTransferred;

    onDataTransfer(_bytesTransferred, Meta_.size_);
}

void FileSharingBlockBase::fileSharingUploadingResult(const QString& _seq, bool /*_success*/, const QString& localPath, const QString& _uri, int /*_contentType*/, qint64 /*_size*/, qint64 /*_lastModified*/, bool _isFileTooBig)
{
    if (_seq != uploadId_)
        return;

    if (_isFileTooBig)
    {
        Q_EMIT removeMe();
        return;
    }

    loadedFromLocal_ = false;
    Link_ = _uri;
    fileId_ = extractIdFromFileSharingUri(getLink());
    setFileName(QFileInfo(localPath).fileName());
    setSourceText(_uri);
    uploadId_.clear();

    Data::FileSharingDownloadResult result;
    result.rawUri_ = _uri;
    result.filename_ = localPath;

    onFileDownloaded(DownloadRequestId_, result);

    Q_EMIT uploaded(QPrivateSignal());

    requestMetainfo(false);
}

bool FileSharingBlockBase::onMenuItemTriggered(const QVariantMap& _params)
{
    const auto command = _params[qsl("command")].toString();
    if (command == u"dev:copy_fs_id")
    {
        QApplication::clipboard()->setText(getFileSharingId().fileId_);
        showToast(QT_TRANSLATE_NOOP("chat_page", "Filesharing Id copied to clipboard"));
    }
    else if (command == u"dev:copy_fs_link")
    {
        QApplication::clipboard()->setText(getLink());
        showToast(QT_TRANSLATE_NOOP("chat_page", "Filesharing link copied to clipboard"));
    }

    return GenericBlock::onMenuItemTriggered(_params);
}

IItemBlock::MenuFlags FileSharingBlockBase::getMenuFlags(QPoint _p) const
{
    int flags = GenericBlock::getMenuFlags(_p);
    if (Meta_.savedByUser_ && QFileInfo::exists(Meta_.localPath_))
        flags |= IItemBlock::MenuFlags::MenuFlagOpenFolder;

    return IItemBlock::MenuFlags(flags);
}

PinPlaceholderType FileSharingBlockBase::getPinPlaceholder() const
{
    if (getType().is_sticker())
        return PinPlaceholderType::Sticker;

    if (isPreviewable())
        return isImage() ? PinPlaceholderType::FilesharingImage : PinPlaceholderType::FilesharingVideo;

    return PinPlaceholderType::Filesharing;
}

UI_COMPLEX_MESSAGE_NS_END
