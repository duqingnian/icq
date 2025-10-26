#pragma once

#include "../../../namespaces.h"

#include "GenericBlock.h"
#include "types/filesharing_meta.h"
#include "types/filesharing_download_result.h"
#include "../FileStatus.h"

namespace Ui
{
    enum class MediaType;
}

CORE_NS_BEGIN
struct file_sharing_content_type;
CORE_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

class IFileSharingBlockLayout;

class FileSharingBlockBase : public GenericBlock
{
    Q_OBJECT

public:
    FileSharingBlockBase(
        ComplexMessageItem *_parent,
        const QString &_link,
        const core::file_sharing_content_type& _type);

    virtual ~FileSharingBlockBase() override;

    virtual void clearSelection() override;

    virtual QString formatRecentsText() const final override;

    Ui::MediaType getMediaType() const final override;

    virtual IItemBlockLayout* getBlockLayout() const override;

    virtual QString getProgressText() const;

    Data::FString getSourceText() const override;

    Data::FString getTextInstantEdit() const override;

    virtual QString getPlaceholderText() const override;

    virtual Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    virtual void initialize() final override;

    virtual bool isBubbleRequired() const override;

    virtual bool isMarginRequired() const override;

    virtual bool isSelected() const override;

    virtual bool isAllSelected() const override { return isSelected(); }

    virtual ContentType getContentType() const override { return ContentType::FileSharing; }

    bool onMenuItemTriggered(const QVariantMap& _params) override;
    IItemBlock::MenuFlags getMenuFlags(QPoint _p) const override;

    PinPlaceholderType getPinPlaceholder() const override;

    const QString& getFileLocalPath() const;

    int64_t getLastModified() const;

    const QString& getFilename() const;

    int64_t getFileSize() const;
    int64_t getBytesTransferred() const;

    const QString& getLink() const override;

    const QString& getDirectUri() const;

    bool isFileDownloaded() const;

    bool isFileDownloading() const;

    bool isFileUploading() const;

    bool isGifImage() const;

    bool isLottie() const;

    bool isImage() const;

    bool isPtt() const;

    bool isPlainFile() const;

    bool isPreviewable() const override;

    bool isPlayable() const;
    bool isPlayableType() const;

    bool isVideo() const;

    Data::FilesPlaceholderMap getFilePlaceholders() override;

    bool isDownloadAllowed() const;
    bool isBlockedFileSharing() const override;
    bool isFileSharingWithStatus() const override;

    bool isFileTrustAllowed() const;

    bool isVirusInfected() const;
    bool isAllowedByAntivirus() const override;
    core::antivirus::check antivirusCheckState() const;

    bool isDraggable() const override;

    void processMetaInfo(const Utils::FileSharingId& _fsId, const Data::FileSharingMeta& _meta);
    void processMetaInfoError(const Utils::FileSharingId& _fsId, qint32 _errorCode);

Q_SIGNALS:
    void uploaded(QPrivateSignal) const;

protected:

    IFileSharingBlockLayout* getFileSharingLayout() const;

    const Utils::FileSharingId& getFileSharingId() const;

    const core::file_sharing_content_type& getType() const;

    virtual void initializeFileSharingBlock() = 0;

    virtual void onDataTransferStarted() = 0;

    virtual void onDataTransferStopped() = 0;

    virtual void onDownloaded() = 0;

    virtual void onDownloadedAction() = 0;

    virtual void onDataTransfer(const int64_t _bytesTransferred, const int64_t _bytesTotal, bool _showBytes = true) = 0;

    virtual void onDownloadingFailed(const int64_t _requestId) = 0;

    virtual void onLocalCopyInfoReady(const bool _isCopyExists) = 0;

    virtual void onMenuCopyLink() final override;

    virtual void onMenuCopyFile() final override;

    virtual void onMenuSaveFileAs() final override;

    virtual void onMenuOpenInBrowser() final override;

    void showFileInDir(const Utils::OpenAt _inFolder) override;

    virtual void onMetainfoDownloaded() = 0;

    virtual void onPreviewMetainfoDownloaded() = 0;

    void requestMetainfo(bool _isPreview);

    void setBlockLayout(IFileSharingBlockLayout* _layout);

    void setSelected(const bool _isSelected) override;

    enum class Priority
    {
        Low,
        High
    };
    void startDownloading(const bool _sendStats, Priority Priority = Priority::Low);

    void stopDataTransfer();

    int64_t getPreviewMetaRequestId() const;
    void clearPreviewMetaRequestId();

    virtual void cancelRequests() override;

    void setFileName(const QString& _filename);

    void setFileSize(int64_t _size);

    void setLocalPath(const QString& _localPath);

    void setLoadedFromLocal(bool _value);

    void setUploadId(const QString& _id);

    bool isInited() const;

    QStringList messageLinks() const override;

private:
    void connectSignals();

    bool isDownload(const Data::FileSharingDownloadResult& _result) const;

    QString getPlaceholderFormatText() const;

    void startDataTransferTimeoutTimer();
    void stopDataTransferTimeoutTimer();

    QString Link_;
    QString SaveAs_;
    QString uploadId_;
    Utils::FileSharingId fileId_;
    IFileSharingBlockLayout *Layout_ = nullptr;
    QTimer* dataTransferTimeout_ = nullptr;
    std::unique_ptr<core::file_sharing_content_type> Type_;
    int64_t BytesTransferred_ = -1;
    int64_t DownloadRequestId_ = -1;
    int64_t FileMetaRequestId_ = -1;
    int64_t fileDirectLinkRequestId_ = -1;
    int64_t PreviewMetaRequestId_ = -1;
    std::function<void()> delayedCallDownloading_;
    bool CopyFile_ = false;
    bool IsSelected_ = false;
    bool inited_ = false;
    bool loadedFromLocal_ = false;

protected:
    Data::FileSharingMeta Meta_;
    FileStatus status_;

private Q_SLOTS:
    void onFileDownloaded(qint64 _seq, const Data::FileSharingDownloadResult& _result);

    void onFileDownloading(qint64 _seq, QString _rawUri, qint64 _bytesTransferred, qint64 _bytesTotal);

    void onFileSharingError(qint64 _seq, QString _rawUri, qint32 _errorCode);

    void onPreviewMetainfoDownloadedSlot(qint64 _seq, QString _miniPreviewUri, QString _fullPreviewUri);

    void fileSharingUploadingProgress(const QString& _uploadingId, qint64 _bytesTransferred);

    void fileSharingUploadingResult(const QString& _seq, bool _success, const QString& _localPath, const QString& _uri, int _contentType, qint64 _size, qint64 _lastModified, bool _isFileTooBig);
};

UI_COMPLEX_MESSAGE_NS_END
