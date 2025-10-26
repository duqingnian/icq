#pragma once

#include "../../corelib/enumerations.h"
#include "../types/message.h"
#include "../types/filesharing_download_result.h"
#include "../controls/TextUnit.h"
#include "StringComparator.h"
#include "UrlUtils.h"
#include "SvgUtils.h"
#include "../styles/ThemeParameters.h"

class QApplication;
class QScreen;

#ifdef _WIN32
QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int hbitmapFormat = 0);
#endif //_WIN32

namespace Ui
{
    class GeneralDialog;
    class CheckBox;
    class MessagesScrollArea;
    class HistoryControlPage;
    class PageBase;
    enum class ConferenceType;
}

namespace Debug
{
    void debugFormattedText(QMimeData* _mime);
    void dumpQtEvent(QEvent* _event, QStringView _context = {});

    template <typename This, typename FuncName>
    void debugMemberCallImpl(This _this, FuncName _funcName)
    {
#ifdef _DEBUG
        qDebug() << typeid(*_this).name() << _funcName;
#else
        Q_UNUSED(_this)
        Q_UNUSED(_funcName)
#endif
    }

    template <typename This, typename FuncName, typename Arg>
    void debugMemberCallImpl1(This _this, FuncName _funcName, Arg _arg)
    {
#ifdef _DEBUG
        qDebug() << typeid(*_this).name() << _funcName << _arg;
#else
        Q_UNUSED(_this)
        Q_UNUSED(_funcName)
#endif
    }

    template <typename This, typename FuncName, typename Arg1, typename Arg2>
    void debugMemberCallImpl2(This _this, FuncName _funcName, Arg1 _arg1, Arg2 _arg2)
    {
#ifdef _DEBUG
        qDebug() << typeid(*_this).name() << _funcName << _arg1 << _arg2;
#else
        Q_UNUSED(_this)
        Q_UNUSED(_funcName)
#endif
    }
}

#define imDebugMemberCall Debug::debugMemberCallImpl(this, __func__)
#define imDebugMemberCall1(x) Debug::debugMemberCallImpl1(this, __func__, x)
#define imDebugMemberCall2(x, y) Debug::debugMemberCallImpl2(this, __func__, x, y)

namespace Utils
{
    namespace Exif
    {
        enum class ExifOrientation;
    }

    inline void ensureMainThread() { im_assert(qApp && QThread::currentThread() == qApp->thread()); }
    inline void ensureNonMainThread() { im_assert(qApp && QThread::currentThread() != qApp->thread()); }

    QString getAppTitle();
    QString getVersionLabel();
    QString getVersionPrintable();

    struct QStringHasher
    {
        std::size_t operator()(const QString& _k) const noexcept
        {
            return qHash(_k);
        }
    };

    struct QStringViewHasher
    {
        std::size_t operator()(QStringView _k) const noexcept
        {
            return qHash(_k);
        }
    };

    struct QColorHasher
    {
        std::size_t operator()(const QColor& _k) const noexcept
        {
            return qHash(_k.rgba());
        }
    };

    void elideText(QString& _text, int _length);

    template <typename T>
    class [[nodiscard]] ScopeExitT
    {
    public:
        ScopeExitT() = delete;
        ScopeExitT(ScopeExitT&&) = delete;
        ScopeExitT(const ScopeExitT&) = delete;
        ScopeExitT(T t) : f(std::move(t)) {}
        ~ScopeExitT()
        {
            f();
        }

    private:
        T f;
    };

    // Searches for an ancestor of a particular type
    template<class T>
    bool hasAncestorOf(const QObject* _object)
    {
        using ObjType = typename std::remove_cv<typename std::remove_pointer<T>::type>::type;
        bool found = false;
        const QObject* parent = _object ? _object->parent() : nullptr;
        while (parent && !found) // walk up in parent-child chain
        {
            if (qobject_cast<const ObjType*>(parent))
                found = true;
            parent = parent->parent();
        }
        return found;
    }

    class ShadowWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ShadowWidgetEventFilter(QObject* _parent, int _shadowWidth, double _radius = 0.0);

    protected:
        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;

    private:
        void setGradientColor(QGradient& _gradient, bool _isActive);

    private:
        int ShadowWidth_;
        int Radius_;
    };

    class [[nodiscard]] PainterSaver
    {
    public:
        Q_DISABLE_COPY_MOVE(PainterSaver)

        PainterSaver(QPainter* _painter)
            : painter_(_painter)
        {
            im_assert(_painter != nullptr);
            if (painter_)
                painter_->save();
        }

        PainterSaver(QPainter& _painter)
            : painter_(&_painter)
        {
            painter_->save();
        }

        ~PainterSaver() noexcept
        {
            if (painter_)
                painter_->restore();
        }
    private:
        QPainter* painter_;
    };

    bool isUin(const QString& _aimId);

    QString getCountryNameByCode(const QString& _iso_code);
    QString getCountryCodeByName(const QString& _name);

    QMap<QString, QString> getCountryCodes();
    // Stolen from Formatter in history_control , move it here completely?
    QString formatFileSize(const int64_t size);

    QString ScaleStyle(const QString& _style, double _scale);

    void SetProxyStyle(QWidget* _widget, QStyle* _style);
    QString prepareStyle(const QString& _style);
    void ApplyStyle(QWidget* _widget, const QString& _style);
    void ApplyPropertyParameter(QWidget* _widget, const char* _property, QVariant _parameter);

    QString LoadStyle(const QString& _qssFile);

    QPixmap getDefaultAvatar(const QString& _uin, const QString& _displayName, const int _sizePx);
    QColor getNameColor(const QString& _uin);

    std::vector<std::vector<QString>> GetPossibleStrings(const QString& _text, unsigned& _count);

    QPixmap roundImage(const QPixmap& _img, bool _isDefault, bool _miniIcons);

    void addShadowToWidget(QWidget* _target);
    void addShadowToWindow(QWidget* _target, bool _enabled = true, double _radius = 0.0);

    void grabTouchWidget(QWidget* _target, bool _topWidget = false);

    void removeLineBreaks(QString& _source);

    bool isValidEmailAddress(const QString& _email);

    bool foregroundWndIsFullscreened();

    [[nodiscard]] double fscale_value(const double _px) noexcept;
    [[nodiscard]] int scale_value(const int _px) noexcept;
    [[nodiscard]] QSize scale_value(const QSize& _px) noexcept;
    [[nodiscard]] QSizeF scale_value(const QSizeF& _px) noexcept;
    [[nodiscard]] QRect scale_value(const QRect& _px) noexcept;
    [[nodiscard]] QPoint scale_value(const QPoint& _px) noexcept;
    [[nodiscard]] QMargins scale_value(const QMargins& _px) noexcept;

    [[nodiscard]] int unscale_value(int _px) noexcept;
    [[nodiscard]] QSize unscale_value(const QSize& _px) noexcept;
    [[nodiscard]] QRect unscale_value(const QRect& _px) noexcept;
    [[nodiscard]] QPoint unscale_value(const QPoint& _px) noexcept;

    [[nodiscard]] int scale_bitmap_ratio() noexcept;
    [[nodiscard]] int scale_bitmap(const int _px) noexcept;
    [[nodiscard]] double fscale_bitmap(const double _px) noexcept;
    [[nodiscard]] QSize scale_bitmap(const QSize& _px) noexcept;
    [[nodiscard]] QSizeF scale_bitmap(const QSizeF& _px) noexcept;
    [[nodiscard]] QRect scale_bitmap(const QRect& _px) noexcept;

    [[nodiscard]] int unscale_bitmap(const int _px) noexcept;
    [[nodiscard]] QSize unscale_bitmap(const QSize& _px) noexcept;
    [[nodiscard]] QRect unscale_bitmap(const QRect& _px) noexcept;

    [[nodiscard]] int scale_bitmap_with_value(const int _px) noexcept;
    [[nodiscard]] double fscale_bitmap_with_value(const double _px) noexcept;
    [[nodiscard]] QSize scale_bitmap_with_value(const QSize& _px) noexcept;
    [[nodiscard]] QSizeF scale_bitmap_with_value(const QSizeF& _px) noexcept;
    [[nodiscard]] QRect scale_bitmap_with_value(const QRect& _px) noexcept;
    [[nodiscard]] QRectF scale_bitmap_with_value(const QRectF& _px) noexcept;

    int getBottomPanelHeight();
    int getTopPanelHeight();

    constexpr int minSide(const QSize& _s) noexcept { return std::min(_s.width(), _s.height()); }
    constexpr int minSide(const QRect& _r) noexcept { return minSide(_r.size()); }

    constexpr qreal minSide(const QSizeF& _s) noexcept { return std::min(_s.width(), _s.height()); }
    constexpr qreal minSide(const QRectF& _r) noexcept { return minSide(_r.size()); }

    constexpr int maxSide(const QSize& _s) noexcept { return std::max(_s.width(), _s.height()); }
    constexpr int maxSide(const QRect& _r) noexcept { return maxSide(_r.size()); }

    constexpr qreal maxSide(const QSizeF& _s) noexcept { return std::max(_s.width(), _s.height()); }
    constexpr qreal maxSide(const QRectF& _r) noexcept { return maxSide(_r.size()); }

    constexpr int text_sy() noexcept // spike for macos drawtext
    {
        return platform::is_apple() ? 2 : 0;
    }

    template <typename _T>
    void check_pixel_ratio(_T& _image);

    template <typename _T>
    void check_pixel_ratio(const _T&&) = delete;

    QString parse_image_name(const QString& _imageName);



    bool is_mac_retina() noexcept;
    void set_mac_retina(bool _val) noexcept;
    double getScaleCoefficient() noexcept;
    void setScaleCoefficient(double _coefficient) noexcept;
    double getBasicScaleCoefficient() noexcept;
    void initBasicScaleCoefficient(double _coefficient) noexcept;

    void groupTaskbarIcon(bool _enabled);

    bool isStartOnStartup();
    void setStartOnStartup(bool _start);

#ifdef _WIN32
    HWND createFakeParentWindow();
#endif //WIN32

    constexpr uint getInputMaximumChars() { return 10000; }

    int calcAge(const QDateTime& _birthdate);

    void drawText(QPainter& painter, const QPointF& point, int flags, const QString& text, QRectF* boundingRect = nullptr);

    const std::vector<QStringView>& getImageExtensions();
    const std::vector<QStringView>& getVideoExtensions();

    bool is_image_extension(QStringView _ext);
    bool is_image_extension_not_gif(QStringView _ext);
    bool is_video_extension(QStringView _ext);

    void copyFileToClipboard(const QString& _path);
    void copyLink(const QString& _link);

    void saveAs(const QString& _inputFilename, std::function<void (const QString& _filename, const QString& _directory, bool _exportAsPng)> _callback, std::function<void ()> _cancel_callback = std::function<void ()>(), bool asSheet = true /* for OSX only */);

    template<typename T>
    inline QByteArray imageToData(const T& _pixmap, const char* _format = nullptr, int _quality = -1)
    {
        QByteArray array;
        QBuffer b(&array);
        b.open(QIODevice::WriteOnly);

        // WebP format can only works in RGBA8888 mode, but it's endian
        // dependant, and Qt webp image handler does this conversion for
        // us behind the scenes, but this could lead to incorrect colors
        // in the resulting image. The following code forces the Big/Little
        // Endianess conversion for original image by simply drawing
        // original image into temporary image with correct RGBA8888 format.
        // Also the PNG format can have custom iCCC profile, and that can lead
        // to the same incorrect visual colors of the image.
        // Note that we use a premultiplied format version to speed-up painting.
        QImage image(_pixmap.size(), QImage::Format_RGBA8888_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        if constexpr (std::is_same_v<T, QPixmap>)
            painter.drawPixmap(0, 0, _pixmap);
        else
            painter.drawImage(0, 0, _pixmap);
        image.save(&b, _format, _quality);

        return array;
    }

    bool canCovertToWebp(const QString& _path);

    using SendKeysIndex = std::vector<std::pair<QString, Ui::KeyToSendMessage>>;

    const SendKeysIndex& getSendKeysIndex();

    using ShortcutsCloseActionsList = std::vector<std::pair<QString, Ui::ShortcutsCloseAction>>;
    const ShortcutsCloseActionsList& getShortcutsCloseActionsList();
    QString getShortcutsCloseActionName(Ui::ShortcutsCloseAction _action);
    using ShortcutsSearchActionsList = std::vector<std::pair<QString, Ui::ShortcutsSearchAction>>;
    const ShortcutsSearchActionsList& getShortcutsSearchActionsList();

    using PrivacyAllowVariantsList = std::vector<std::pair<QString, core::privacy_access_right>>;
    const PrivacyAllowVariantsList& getPrivacyAllowVariants();

    void post_stats_with_settings();
    QRect GetMainRect();
    QPoint GetMainWindowCenter();
    QRect GetWindowRect(QWidget* window);

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields);

    Ui::GeneralDialog *NameEditorDialog(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool NameEditor(
        QWidget* _parent,
        const QString& _chatName,
        const QString& _buttonText,
        const QString& _headerText,
        Out QString& resultChatName,
        bool acceptEnter = true);

    bool UrlEditor(
        QWidget* _parent,
        QStringView _linkDisplayName,
        InOut QString& _url);

    bool GetConfirmationWithTwoButtons(
        const QString& _buttonLeft,
        const QString& _buttonRight,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow = nullptr,
        bool _withSemiwindow = true);

    bool GetDeleteConfirmation(
        const QString& _buttonLeft,
        const QString& _buttonRight,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow = nullptr,
        bool _withSemiwindow = true);

    bool GetConfirmationWithOneButton(
        const QString& _buttonText,
        const QString& _messageText,
        const QString& _labelText,
        QWidget* _parent,
        QWidget* _mainWindow = nullptr,
        bool _withSemiwindow = true);

    bool GetErrorWithTwoButtons(
        const QString& _buttonLeftText,
        const QString& _buttonRightText,
        const QString& _messageText,
        const QString& _labelText,
        const QString& _errorText,
        QWidget* _parent);

    void ShowBotAlert(const QString& _alertText);

    void showCancelGroupJoinDialog(const QString& _aimId);

    struct ProxySettings
    {
        const static int invalidPort = -1;

        core::proxy_type type_;
        core::proxy_auth authType_;
        QString username_;
        bool needAuth_;
        QString password_;
        QString proxyServer_;
        int port_;

        ProxySettings(core::proxy_type _type,
                      core::proxy_auth _authType,
                      QString _username,
                      QString _password,
                      QString _proxy,
                      int _port,
                      bool _needAuth);

        ProxySettings();

        void postToCore();

        static QString proxyTypeStr(core::proxy_type _type);

        static QString proxyAuthStr(core::proxy_auth _type);

        static QNetworkProxy::ProxyType proxyType(core::proxy_type _type);
    };

    ProxySettings* get_proxy_settings();

    QSize getMaxImageSize();

    QScreen* mainWindowScreen();

    QPixmap loadPixmap(const QString& _resource);

    bool loadPixmap(const QString& _path, Out QPixmap& _pixmap);

    bool loadPixmap(const QByteArray& _data, Out QPixmap& _pixmap, Exif::ExifOrientation _orientation);

    enum class PanoramicCheck
    {
        no,
        yes,
    };
    bool loadPixmapScaled(const QString& _path, const QSize& _maxSize, Out QPixmap& _pixmap, Out QSize& _originalSize, const PanoramicCheck _checkPanoramic = PanoramicCheck::yes);

    bool loadImageScaled(const QString& _path, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize, const PanoramicCheck _checkPanoramic = PanoramicCheck::yes);

    bool loadPixmapScaled(QByteArray& _data, const QSize& _maxSize, Out QPixmap& _pixmap, Out QSize& _originalSize);

    bool loadImageScaled(QByteArray& _data, const QSize& _maxSize, Out QImage& _image, Out QSize& _originalSize);

    bool loadBase64Image(const QByteArray& _data, const QSize& _maxSize, Out QImage& image, Out QSize& _originalSize);

    struct ImageBase64Format
    {
        QByteArray imageFormat;
        int headerLength;
    };

    ImageBase64Format detectBase64ImageFormat(const QByteArray& _data);

    bool isBase64EncodedImage(const QString& _data);

    bool isBase64(const QByteArray& _data);

    bool dragUrl(QObject* _parent, const QPixmap& _preview, const QString& _url);

    class StatsSender : public QObject
    {
        Q_OBJECT
    public :
        StatsSender();

    public Q_SLOTS:
        void recvGuiSettings() { guiSettingsReceived_ = true; trySendStats(); }
        void recvThemeSettings() { themeSettingsReceived_ = true; trySendStats(); }

    public:
        void trySendStats() const;

    private:
        bool guiSettingsReceived_;
        bool themeSettingsReceived_;
    };

    StatsSender* getStatsSender();

    bool haveText(const QMimeData *);

    std::string_view get_crossprocess_mutex_name();

    QHBoxLayout* emptyHLayout(QWidget* parent = nullptr);
    QVBoxLayout* emptyVLayout(QWidget* parent = nullptr);
    QGridLayout* emptyGridLayout(QWidget* parent = nullptr);

    void emptyContentsMargins(QWidget* w);
    void transparentBackgroundStylesheet(QWidget* w);

    QString getProductName();
    QString getInstallerName();

    QString getUnreadsBadgeStr(const int _unreads);

    QSize getUnreadsBadgeSize(const int _unreads, const int _height);

    QPixmap getUnreadsBadge(const int _unreads, const QColor _bgColor, const int _height);

    int drawUnreads(
        QPainter& _p,
        const QFont& _font,
        const QColor& _bgColor,
        const QColor& _textColor,
        const int _unreadsCount,
        const int _badgeHeight,
        const int _x,
        const int _y);

    int drawUnreads(
        QPainter *p, const QFont &font, QColor bgColor, QColor textColor, QColor borderColor,
        int unreads, int balloonSize, int x, int y, QPainter::CompositionMode borderMode = QPainter::CompositionMode_SourceOver);

    QPixmap pixmapWithEllipse(const QPixmap& _original, const QColor& _brushColor, int brushWidth);

    namespace Badge
    {
        enum class Color
        {
            Red,
            Green,
            Gray
        };

        QSize getSize(int _textLength, bool _isTab = false);
        int drawBadge(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _right, int _top, Color _color,
                      const QColor& _borderColor = Styling::getParameters().getColor(Styling::StyleVariable::BASE_GLOBALWHITE), bool _isTab = false);
        int drawBadgeRight(const std::unique_ptr<Ui::TextRendering::TextUnit>& _textUnit, QPainter& _p, int _right, int _top, Color _color);
    }

    QSize getUnreadsSize(const QFont& _font, const bool _withBorder, const int _unreads, const int _balloonSize);

    QImage iconWithCounter(int size, int count, QColor bg, QColor fg, QImage back = QImage());

    QStringView extractAimIdFromMention(QStringView _mention);


    [[nodiscard]] QString convertMentions(const QString& _source, const Data::MentionMap& _mentions);
    void convertMentions(Data::FString& _source, const Data::MentionMap& _mentions);

    //! Keeps other styles on mentions
    void convertMentionsToFriendlyPlainText(Data::FString& _source, const Data::MentionMap& _mentions);

    //! Replace ` and ``` by format if text is plaintext (mentions do not count)
    void convertOldStyleMarkdownToFormats(Data::FString& _text, Ui::ParseBackticksPolicy _backticksPolicy = Ui::ParseBackticksPolicy::ParseSinglesAndTriples);

    void convertMentions(
        Data::FString& _source,
        std::function<Data::FString(const core::data::range_format&, Data::FStringView)> _converter);

    [[nodiscard]] QString convertFilesPlaceholders(const QStringRef& _source, const Data::FilesPlaceholderMap& _files);

    inline QString convertFilesPlaceholders(const QString& _source, const Data::FilesPlaceholderMap& _files)
    {
        return convertFilesPlaceholders(QStringRef(&_source), _files);
    }

    void convertFilesPlaceholders(Data::FString& _source, const Data::FilesPlaceholderMap& _files);

    [[nodiscard]] QString replaceFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files);
    void replaceFilesPlaceholders(Data::FString& _text, const Data::FilesPlaceholderMap& _files);

    [[nodiscard]] QString setFilesPlaceholders(QString _text, const Data::FilesPlaceholderMap& _files);
    [[nodiscard]] Data::FString setFilesPlaceholders(const Data::FString& _text, const Data::FilesPlaceholderMap& _files);

    void clearContentCache();
    void clearAvatarsCache();
    void removeOmicronFile();
    void checkForUpdates();

    enum class OpenDOPResult
    {
        dialog,
        profile,
    };

    enum class OpenDOPParam
    {
        aimid,
        stamp,
    };

    OpenDOPResult openDialogOrProfile(const QString& _contact, const OpenDOPParam _paramType = OpenDOPParam::aimid);
    void openDialogWithContact(const QString& _contact, qint64 _id = -1, bool _sel = true, std::function<void (Ui::PageBase*)> _getPageCallback = nullptr);
    void openFeedContact(const QString& _contact);
    void updateAppsPageReopenPage(const QString& _contact);

    bool clicked(const QPoint& _prev, const QPoint& _cur, int dragDistance = 0);

    void drawBubbleRect(QPainter& _p, const QRectF& _rect, const QColor& _color, int _bWidth, int _bRadious);

    int getShadowMargin() noexcept;
    void drawBubbleShadow(QPainter& _p, const QPainterPath& _bubble, const int _clipLength = -1, const int _shadowMargin = -1, const QColor _shadowColor = QColor());

    //! Used for reaction and thread plates
    QGraphicsDropShadowEffect* initPlateShadowEffect(QWidget* _parent, double _opacity = 1.0);

    QColor plateShadowColorWithAlpha(double _opacity = 1.0);

    void drawPlateSolidShadow(QPainter& _p, const QPainterPath& _path);

    enum class StatusBadgeState
    {
        CanBeOff,
        AlwaysOn,
        StatusOnly,
        BadgeOnly,
        Hovered,
        Pressed
    };

    struct StatusBadge
    {
        QPoint offset_;
        QSize size_;

        StatusBadge(int _offset = 0, int _statusSize = 0)
            : offset_(Utils::scale_value(QPoint(_offset, _offset)))
            , size_(Utils::scale_value(QSize(_statusSize, _statusSize)))
        {};

        bool isValid() const { return !size_.isNull(); }
    };

    QSize avatarWithBadgeSize(const int _avatarWidthScaled);
    const StatusBadge& getStatusBadgeParams(const int _avatarWidthScaled, bool _small = true);
    QPixmap getStatusBadge(const QString& _aimid, int _avatarSize, bool _small = true);
    QPixmap getBotDefaultBadge(int _badgeSize);
    QPixmap getEmptyStatusBadge(StatusBadgeState _state, int _avatarSize);
    struct AvatarBadge
    {
        QSize size_;
        QPoint offset_;
        QPixmap officialIcon_;
        QPixmap muteIcon_;
        QPixmap smallOnlineIcon_;
        QPixmap smallSelectedOnlineIcon_;
        QPixmap onlineIcon_;
        QPixmap onlineSelectedIcon_;
        int cutWidth_;
        int emojiOffsetY_;
        AvatarBadge(){};
        AvatarBadge(const int _size, const int _offsetX, const int _offsetY,
                    const int _cutWidth, const QString& _officialPath,
                    const QString& _mutedPath, const QString& _smallOnlinePath, const QString& _onlinePath, const QString& _onlineSelectedPath);
        bool isEmpty() const { return size_ == QSize(0, 0); };
    };
    AvatarBadge getBadgeData(int _avatarWidth);

    enum class StatusBadgeFlag
    {
        None        = 0,
        Official    = 1 << 1,
        Muted       = 1 << 2,
        Selected    = 1 << 3,
        Online      = 1 << 4,
        SmallOnline = 1 << 5,
        WithOverlay = 1 << 6,
        Small       = 1 << 7,
    };
    Q_DECLARE_FLAGS(StatusBadgeFlags, StatusBadgeFlag)
    Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::StatusBadgeFlags)

    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm, const QPixmap& _status, const StatusBadgeFlags _flags = StatusBadgeFlag::Small);
    void drawAvatarWithBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm, const QString& _aimid, StatusBadgeState _state = StatusBadgeState::CanBeOff, const StatusBadgeFlags _flags = StatusBadgeFlag::Small | StatusBadgeFlag::SmallOnline);
    void drawAvatarWithoutBadge(QPainter& _p, const QPoint& _topLeft, const QPixmap& _pm, const QPixmap& _status = QPixmap());

    template<typename T>
    QString replaceLine(T&& _s)
    {
        return std::forward<T>(_s).simplified();
    }

    enum MirrorDirection
    {
        vertical = 0x1,
        horizontal = 0x2,
        both = vertical | horizontal
    };

    QPixmap mirrorPixmap(const QPixmap& _pixmap, const MirrorDirection _direction);

    inline QPixmap mirrorPixmapHor(const QPixmap& _pixmap)
    {
        return mirrorPixmap(_pixmap, MirrorDirection::horizontal);
    }

    inline QPixmap mirrorPixmapVer(const QPixmap& _pixmap)
    {
        return mirrorPixmap(_pixmap, MirrorDirection::vertical);
    }

    void drawUpdatePoint(QPainter& _p, const QPoint &_center, int _radius, int _borderRadius);

    void restartApplication();

    struct CloseWindowInfo
    {
        enum class Initiator
        {
            MainWindow = 0,
            MacEventFilter = 1,
            Unknown = -1
        };

        enum class Reason
        {
            MW_Resizing = 0,
            Keep_Sidebar,
            MacEventFilter,
            ShowLoginPage,
            ShowGDPRPage,
            ShowUserAgreementPage
        };

        Initiator initiator_ = Initiator::Unknown;
        Reason reason_ = Reason::MW_Resizing;

        CloseWindowInfo() = default;
        CloseWindowInfo(Initiator _initiator, Reason _reason) : initiator_(_initiator), reason_(_reason) {}

        bool operator==(const CloseWindowInfo& _other) const
        {
            return initiator_ == _other.initiator_ && reason_ == _other.reason_;
        }
    };

    class PhoneValidator : public QValidator
    {
        Q_OBJECT
    public:
        PhoneValidator(QWidget* _parent, bool _isCode, bool _isParseFullNumber = false);
        virtual QValidator::State validate(QString& input, int&) const override;

    private:
        bool isCode_;
        bool isParseFullNumber_;
    };

    class DeleteMessagesWidget : public QWidget
    {
        Q_OBJECT
    public:

        enum ShowInfoText
        {
            Yes,
            No
        };

        DeleteMessagesWidget(QWidget* _parent, bool _showCheckBox, ShowInfoText _showInfoText = ShowInfoText::Yes);
        bool isChecked() const;

        void setCheckBoxText(const QString& _text);
        void setCheckBoxChecked(bool _value);
        void setInfoText(const QString& _text);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;

    private:
        Ui::CheckBox* checkbox_;
        std::unique_ptr<Ui::TextRendering::TextUnit> label_;
        const bool showCheckBox_;
        const ShowInfoText showInfoText_;
    };

    class CheckableInfoWidget : public QWidget
    {
        Q_OBJECT
    public:
        explicit CheckableInfoWidget(QWidget* _parent);
        bool isChecked() const;

        void setCheckBoxText(const QString& _text);
        void setCheckBoxChecked(bool _value);
        void setInfoText(const QString& _text, const Styling::ThemeColorKey& _color = {});

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;

    private:
        Ui::CheckBox* checkbox_;
        std::unique_ptr<Ui::TextRendering::TextUnit> label_;
        QRect checkboxRect_;
        bool hovered_;
    };

    void logMessage(const QString& _message);

    bool isPanoramic(const QSize& _size);

    void updateBgColor(QWidget* _widget, const QColor& _color);
    void setDefaultBackground(QWidget* _widget);
    void drawBackgroundWithBorders(QPainter& _p, const QRect& _rect, const QColor& _bg, const QColor& _border, const Qt::Alignment _borderSides, const int _borderWidth = Utils::scale_value(1));

    bool startsCyrillic(const QString& _str);
    bool startsNotLetter(const QString& _str);

    QPixmap tintImage(const QPixmap& _source, const QColor& _tint);
    bool isChat(QStringView _aimid);
    bool isServiceAimId(QStringView _aimId);

    QString getStickerBotAimId();

    template<typename T, typename V>
    bool is_less_by_first(const std::pair<T, V>& x1, const std::pair<T, V>& x2)
    {
        static_assert(std::is_same_v<decltype(x1), decltype(x2)>);
        return static_cast<std::underlying_type_t<decltype(x1.first)>>(x1.first) < static_cast<std::underlying_type_t<decltype(x2.first)>>(x2.first);
    }

    QString msgIdLogStr(qint64 _msgId);

    bool canShowSmartreplies(const QString& _aimId);

    QString getDefaultCallAvatarId();

    bool supportUpdates() noexcept;

    enum class CallLinkFrom
    {
        CallLog,
        Chat,
        Input,
        Profile,
    };

    class CallLinkCreator : public QObject
    {
        Q_OBJECT

    public Q_SLOTS:
        void onCreateCallLinkResult(int64_t _seq, int _error, bool _isWebinar, const QString& _url, int64_t _expires);
    public:
        CallLinkCreator(CallLinkFrom _from);
        void createCallLink(Ui::ConferenceType _type, const QString& _aimId);
    private:
        int64_t seqRequestWithLoader_;
        CallLinkFrom from_;
        QString aimId_;
    };

    //! Find index of element in a container. Return -1 if not found
    template<typename T, typename U>
    int indexOf(T start, T end, const U& val)
    {
        int result = -1;
        auto it = std::find(start, end, val);
        if (it != end)
            result = std::distance(start, it);
        return result;
    }

    enum class ROAction
    {
        Ban,
        Allow
    };
    QString getReadOnlyString(ROAction _action, bool _isChannel);
    QString getMembersString(int _number, bool _isChannel);

    QString getFormattedTime(std::chrono::milliseconds _duration);

    constexpr int defaultMouseWheelDelta() noexcept { return 120; }

    bool isLineBreak(QChar _ch);
}

Q_DECLARE_METATYPE(Utils::CloseWindowInfo)

namespace Logic
{
    enum class Placeholder
    {
        Contacts,
        Recents
    };
    void updatePlaceholders(const std::vector<Placeholder> &_locations = {Placeholder::Contacts, Placeholder::Recents});
}

namespace FileSharing
{
    enum class FileType
    {
        archive,
        xls,
        html,
        keynote,
        numbers,
        pages,
        pdf,
        ppt,
        audio,
        txt,
        doc,
        apk,

        unknown,

        max_val,
    };

    FileType getFSType(const QString& _filename);

    QString getPlaceholderForType(FileType _fileType);
}
