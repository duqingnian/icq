#include "stdafx.h"

#include "../controls/CustomButton.h"
#include "../controls/ContextMenu.h"
#include "../main_window/MainWindow.h"
#include "../main_window/containers/FriendlyContainer.h"
#include "../main_window/history_control/complex_message/FileSharingUtils.h"
#include "../main_window/history_control/ActionButtonWidget.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_coll_helper.h"
#include "../utils/stat_utils.h"
#include "../core_dispatcher.h"
#include "../gui_settings.h"
#include "../utils/memory_utils.h"
#include "../main_window/contact_list/FavoritesUtils.h"

#include "../main_window/contact_list/ContactListModel.h"
#include "../main_window/GroupChatOperations.h"
#include "../themes/ResourceIds.h"
#include "../my_info.h"

#include "Drawable.h"
#include "GalleryFrame.h"
#include "ImageViewerWidget.h"

#include "toast.h"

#include "galleryloader.h"
#include "avatarloader.h"

#include "../utils/features.h"
#include "../core_dispatcher.h"

#include "GalleryWidget.h"

#ifdef __APPLE__
#include "utils/macos/mac_support.h"
#endif

namespace
{
    const qreal backgroundOpacity = 0.92;
    const QBrush backgroundColor(QColor(u"#1E1E1E"));
    const int scrollTimeoutMsec = 500;
    const int delayTimeoutMsec = 500;
    const int navButtonWidth = 56;
    const QSize navButtonIconSize = {20, 20};

    enum
    {
        DownloadIndex = 0,
        ImageIndex
    };

    auto getControlsWidgetHeight()
    {
        return Utils::scale_value(162);
    }

    constexpr bool grabKeyboardSwitchEnabled() noexcept
    {
        return platform::is_linux();
    }
}

using namespace Previewer;

GalleryWidget::GalleryWidget(QWidget* _parent)
    : QWidget(_parent)
    , controlWidgetHeight_(getControlsWidgetHeight())
    , antivirusCheckEnabled_{ Features::isAntivirusCheckEnabled() }
    , isClosing_(false)
    , fromThreadFeed_(false)
{
    setMouseTracking(true);

    controlsWidget_ = new GalleryFrame(this);

    controlsWidget_->setSaveEnabled(false);
    controlsWidget_->setMenuEnabled(false);
    controlsWidget_->setPrevEnabled(false);
    controlsWidget_->setNextEnabled(false);
    controlsWidget_->setZoomVisible(false);

    connect(controlsWidget_, &GalleryFrame::needHeight, this, &GalleryWidget::updateControlsHeight);
    connect(controlsWidget_, &GalleryFrame::prev, this, &GalleryWidget::onPrevClicked);
    connect(controlsWidget_, &GalleryFrame::next, this, &GalleryWidget::onNextClicked);
    connect(controlsWidget_, &GalleryFrame::zoomIn, this, &GalleryWidget::onZoomIn);
    connect(controlsWidget_, &GalleryFrame::zoomOut, this, &GalleryWidget::onZoomOut);
    connect(controlsWidget_, &GalleryFrame::forward, this, &GalleryWidget::onForward);
    connect(controlsWidget_, &GalleryFrame::goToMessage, this, &GalleryWidget::onGoToMessage);
    connect(controlsWidget_, &GalleryFrame::saveAs, this, &GalleryWidget::onSaveAs);
    connect(controlsWidget_, &GalleryFrame::saveToFavorites, this, &GalleryWidget::onSaveToFavorites);
    connect(controlsWidget_, &GalleryFrame::close, this, &GalleryWidget::onEscapeClick);
    connect(controlsWidget_, &GalleryFrame::save, this, &GalleryWidget::onSave);
    connect(controlsWidget_, &GalleryFrame::copy, this, &GalleryWidget::onCopy);
    connect(controlsWidget_, &GalleryFrame::openContact, this, &GalleryWidget::onOpenContact);

    connect(new QShortcut(Qt::CTRL + Qt::Key_Equal, this), &QShortcut::activated, this, &GalleryWidget::onZoomIn);
    connect(new QShortcut(Qt::CTRL + Qt::Key_Minus, this), &QShortcut::activated, this, &GalleryWidget::onZoomOut);
    connect(new QShortcut(Qt::CTRL + Qt::Key_0, this), &QShortcut::activated, this, &GalleryWidget::onResetZoom);

    if constexpr (!platform::is_apple())
        connect(new QShortcut(Qt::CTRL + Qt::Key_W, this), &QShortcut::activated, this, &GalleryWidget::escape);

    nextButton_ = new NavigationButton(this);
    prevButton_ = new NavigationButton(this);

    const auto iconSize = Utils::scale_value(navButtonIconSize);
    auto prevPixmap = Utils::renderSvgScaled(qsl(":/controls/back_icon"), iconSize, QColor(255, 255, 255, 0.2 * 255));
    auto nextPixmap = Utils::mirrorPixmapHor(prevPixmap);

    auto prevPixmapHovered = Utils::renderSvgScaled(qsl(":/controls/back_icon"), iconSize, Qt::white);
    auto nextPixmapHovered = Utils::mirrorPixmapHor(prevPixmapHovered);

    nextButton_->setDefaultPixmap(nextPixmap);
    prevButton_->setDefaultPixmap(prevPixmap);

    nextButton_->setHoveredPixmap(nextPixmapHovered);
    prevButton_->setHoveredPixmap(prevPixmapHovered);

    connect(nextButton_, &NavigationButton::clicked, this, &GalleryWidget::onNextClicked);
    connect(prevButton_, &NavigationButton::clicked, this, &GalleryWidget::onPrevClicked);

    imageViewer_ = new ImageViewerWidget(this);
    imageViewer_->setVideoWidthOffset(Utils::scale_value(2 * navButtonWidth));
    connect(imageViewer_, &ImageViewerWidget::zoomChanged, this, &GalleryWidget::updateZoomButtons);
    connect(imageViewer_, &ImageViewerWidget::closeRequested, this, &GalleryWidget::escape);
    connect(imageViewer_, &ImageViewerWidget::clicked, this, &GalleryWidget::clicked);
    connect(imageViewer_, &ImageViewerWidget::fullscreenToggled, this, &GalleryWidget::onFullscreenToggled);
    connect(imageViewer_, &ImageViewerWidget::playClicked, this, &GalleryWidget::onPlayClicked);
    connect(imageViewer_, &ImageViewerWidget::rightClicked, this, [this]()
    {
        trackMenu(QCursor::pos());
    });

    auto rootLayout = Utils::emptyVLayout();
    Testing::setAccessibleName(imageViewer_, qsl("AS GalleryWidget imageViewer"));
    Testing::setAccessibleName(prevButton_, qsl("AS GalleryWidget prevButton"));
    Testing::setAccessibleName(nextButton_, qsl("AS GalleryWidget nextButton"));

    rootLayout->addWidget(imageViewer_);

    setLayout(rootLayout);

    setAttribute(Qt::WA_TranslucentBackground);

    if constexpr (platform::is_linux())
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::BypassWindowManagerHint);
    else if constexpr (platform::is_apple())
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    else
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    scrollTimer_ = new QTimer(this);
    connect(scrollTimer_, &QTimer::timeout, scrollTimer_, &QTimer::stop);

    delayTimer_ = new QTimer(this);
    delayTimer_->setSingleShot(true);
    connect(delayTimer_, &QTimer::timeout, this, &GalleryWidget::onDelayTimeout);

    contextMenuShowTimer_ = new QTimer(this);
    contextMenuShowTimer_->setSingleShot(true);
    contextMenuShowTimer_->setInterval(QApplication::doubleClickInterval());

    QObject::connect(contextMenuShowTimer_, &QTimer::timeout, this, [this]()
    {
        trackMenu(contexMenuPos_, true);
    });

    progressButton_ = new Ui::ActionButtonWidget(Ui::ActionButtonResource::ResourceSet::Play_, this);
    progressButton_->hide();

    connect(progressButton_, &Ui::ActionButtonWidget::stopClickedSignal, this, [this]()
    {
        progressButton_->stopAnimation();
        contentLoader_->cancelCurrentItemLoading();
    });

    connect(progressButton_, &Ui::ActionButtonWidget::startClickedSignal, this, [this]()
    {
        progressButton_->startAnimation();
        contentLoader_->startCurrentItemLoading();
    });

    if constexpr (grabKeyboardSwitchEnabled())
        qApp->installEventFilter(this);

    connect(&Utils::InterConnector::instance(), &Utils::InterConnector::omicronUpdated, this, &GalleryWidget::onConfigChanged);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::externalUrlConfigUpdated, this, &GalleryWidget::onConfigChanged);
}

GalleryWidget::~GalleryWidget()
{
    if constexpr (grabKeyboardSwitchEnabled())
        qApp->removeEventFilter(this);
}

void GalleryWidget::openGallery(const Utils::GalleryData& _data)
{
    aimId_ = _data.aimId_;
    fromThreadFeed_ = _data.fromThreadFeed_;
    const auto hasPlayer = _data.attachedPlayer_ != nullptr;
    const auto hasPreview = !_data.preview_.isNull();
    contentLoader_.reset(createGalleryLoader(_data));

    if (controlsWidget_)
        controlsWidget_->setGotoEnabled(!fromThreadFeed_);

    init();

    if (hasPlayer)
        onMediaLoaded();
    else if (hasPreview)
        onPreviewLoaded();
}

void GalleryWidget::openAvatar(const QString &_aimId)
{
    aimId_ = _aimId;
    contentLoader_.reset(createAvatarLoader(_aimId));

    init();
}

bool GalleryWidget::isClosing() const
{
    return isClosing_;
}

void GalleryWidget::closeGallery()
{
    isClosing_ = true;

    if (contentLoader_)
        contentLoader_->cancelCurrentItemLoading();

    Q_EMIT Utils::InterConnector::instance().setFocusOnInput();
    Ui::ToastManager::instance()->hideToast();

    imageViewer_->reset();
    showNormal();
    close();
    releaseKeyboard();

    Q_EMIT closed();

    // it fixes https://jira.mail.ru/browse/IMDESKTOP-8547, which is most propably caused by https://bugreports.qt.io/browse/QTBUG-49238
    QMetaObject::invokeMethod(this, &GalleryWidget::deleteLater, Qt::QueuedConnection);

    Ui::GetDispatcher()->cancelGalleryHolesDownloading(aimId_);
}

void GalleryWidget::showMinimized()
{
    imageViewer_->hide();

    // todo: fix gallery minimize in macos
    if constexpr (platform::is_apple())
        QWidget::hide();
    else
        QWidget::showMinimized();
}

void GalleryWidget::showFullScreen()
{
#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
#else
    QWidget::showFullScreen();
#ifdef _WIN32
    QWindowsWindowFunctions::setHasBorderInFullScreen(windowHandle(), true);//https://doc.qt.io/qt-5/windows-issues.html
#endif //_WIN32
#endif //__APPLE__

    imageViewer_->show();
}

void GalleryWidget::escape()
{
    if (!imageViewer_->closeFullscreen())
    {
        closeGallery();
        Q_EMIT closed();
    }
}

template <typename ... Args>
void GalleryWidget::addAction(int action, Ui::ContextMenu* menu, Args&& ...args)
{
    const auto type = GalleryFrame::Action(action);
    auto a = menu->addActionWithIcon(GalleryFrame::actionIconPath(type), GalleryFrame::actionText(type), {});
    QObject::connect(a, &QAction::triggered, std::forward<Args>(args)...);
}

void GalleryWidget::trackMenu(const QPoint& _globalPos, const bool _isAsync)
{
    struct WidgetToRestore
    {
        QPointer<QWidget> widget;
        std::function<void()> restore;
    };

    auto prepareWidget = [](auto w)
    {
        if (w)
        {
            if (!w->testAttribute(Qt::WA_TransparentForMouseEvents))
            {
                w->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                return WidgetToRestore{ w, [w]() { w->setAttribute(Qt::WA_TransparentForMouseEvents, false); } };
            }
        }
        return WidgetToRestore{ nullptr };
    };

    auto parent = imageViewer_->getParentForContextMenu();

    auto parentW = prepareWidget(parent);
    auto mainWindowW = prepareWidget(Utils::InterConnector::instance().getMainWindow());

    auto menu = new Ui::ContextMenu(parent, Ui::ContextMenu::Color::Dark);
    menu->setShowAsync(_isAsync);
    Testing::setAccessibleName(menu, qsl("AS GalleryWidget menu"));

    const auto actions = controlsWidget_->menuActions();

    if (const auto type = GalleryFrame::Action::GoTo; (actions & type) && !fromThreadFeed_)
        addAction(type, menu, this, [this]() { onGoToMessage(); closeGallery(); }, Qt::QueuedConnection);

    if (const auto type = GalleryFrame::Action::Copy; actions & type)
        addAction(type, menu, this, &GalleryWidget::onCopy, Qt::QueuedConnection);

    if (const auto type = GalleryFrame::Action::Forward; actions & type)
        addAction(type, menu, this, &GalleryWidget::onForward, Qt::QueuedConnection);

    if (const auto type = GalleryFrame::Action::SaveAs; actions & type)
        addAction(type, menu, this, &GalleryWidget::onSaveAs, Qt::QueuedConnection);

    if (const auto type = GalleryFrame::Action::SaveToFavorites; actions & type)
        addAction(type, menu, this, &GalleryWidget::onSaveToFavorites, Qt::QueuedConnection);

    connect(menu, &Ui::ContextMenu::aboutToHide, this, [parentW, mainWindowW]()
    {
        if (parentW.widget && parentW.restore)
            parentW.restore();
        if (mainWindowW.widget && mainWindowW.restore)
            mainWindowW.restore();
    }, Qt::QueuedConnection);
    connect(menu, &Ui::ContextMenu::aboutToHide, menu, &Ui::ContextMenu::deleteLater, Qt::QueuedConnection);
    connect(this, &GalleryWidget::closeContextMenu, menu, &Ui::ContextMenu::close);
    connect(imageViewer_, &ImageViewerWidget::zoomChanged, menu, &Ui::ContextMenu::close);
    connect(new QShortcut(Qt::CTRL + Qt::Key_0, menu), &QShortcut::activated, this, &GalleryWidget::onResetZoom);
    connect(new QShortcut(Qt::Key_Escape, menu), &QShortcut::activated, menu, &Ui::ContextMenu::close);

    menu->setWheelCallback([this, guard = QPointer(this)](QWheelEvent* _event)
    {
        if (!guard)
            return;
        imageViewer_->tryScale(_event);
    });


    contextMenu_ = menu;
    menu->popup(_globalPos);
}

void GalleryWidget::moveToScreen()
{
    const auto screen = Utils::InterConnector::instance().getMainWindow()->getScreen();
    const auto screenGeometry = QApplication::desktop()->screenGeometry(screen);
    setGeometry(screenGeometry);
}

QString GalleryWidget::getDefaultText() const
{
    return QT_TRANSLATE_NOOP("previewer", "Loading...");
}

QString GalleryWidget::getInfoText(int _n, int _total) const
{
    return QT_TRANSLATE_NOOP("previewer", "%1 of %2").arg(_n).arg(_total);
}

void GalleryWidget::mousePressEvent(QMouseEvent* _event)
{
    if (contextMenu_)
    {
        contextMenu_->close();
        contextMenu_ = {};
    }
    else if (_event->button() == Qt::LeftButton && !controlsWidget_->isCaptionExpanded())
    {
        closeGallery();
    }
}

void GalleryWidget::mouseReleaseEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::LeftButton)
    {
        clicked();
    }
    else if (_event->button() == Qt::RightButton)
    {
        if (std::exchange(skipNextMouseRelease_, false))
            return;
        if (!controlsWidget_->rect().contains(controlsWidget_->mapFromGlobal(_event->globalPos())) && imageViewer_->viewerRect().contains(imageViewer_->mapFromGlobal(_event->globalPos())))
        {
            contexMenuPos_ = _event->globalPos();
            if (!contextMenuShowTimer_->isActive())
                contextMenuShowTimer_->start();
        }
    }
}

void GalleryWidget::mouseDoubleClickEvent(QMouseEvent* _event)
{
    if (_event->button() == Qt::RightButton)
    {
        skipNextMouseRelease_ = true;
        contextMenuShowTimer_->stop();
    }
}

void GalleryWidget::keyPressEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (key == Qt::Key_Escape)
        escape();
    else if (key == Qt::Key_Left && hasPrev())
        prev();
    else if (key == Qt::Key_Right && hasNext())
        next();

    if constexpr (platform::is_linux() || platform::is_windows())
    {
        if (_event->modifiers() == Qt::ControlModifier && key == Qt::Key_Q)
        {
            closeGallery();
            Q_EMIT finished();
        }
    }
}

void GalleryWidget::keyReleaseEvent(QKeyEvent* _event)
{
    const auto key = _event->key();
    if (key == Qt::Key_Plus || key == Qt::Key_Equal)
        onZoomIn();
    else if (key == Qt::Key_Minus)
        onZoomOut();
}

void GalleryWidget::wheelEvent(QWheelEvent* _event)
{
    onWheelEvent(_event->angleDelta());
}

void GalleryWidget::paintEvent(QPaintEvent* /*_event*/)
{
    QPainter painter(this);
    painter.setOpacity(backgroundOpacity);
    painter.fillRect(0, 0, width(), height() - controlWidgetHeight_, backgroundColor);
}

bool GalleryWidget::event(QEvent* _event)
{
    if (_event->type() == QEvent::Gesture)
    {
        auto ge = static_cast<QGestureEvent*>(_event);
        if (QGesture *pinch = ge->gesture(Qt::PinchGesture))
        {
            Q_EMIT closeContextMenu(QPrivateSignal());
            auto pg = static_cast<QPinchGesture *>(pinch);
            imageViewer_->scaleBy(pg->scaleFactor(), QCursor::pos());
            return true;
        }
    }

    return QWidget::event(_event);
}

bool GalleryWidget::eventFilter(QObject* _object, QEvent* _event)
{
    if (_event->type() == QEvent::MouseButtonPress)
    {
        resumeGrabKeyboard();
        activateWindow();
    }
    else if (_event->type() == QEvent::ActivationChange)
    {
        if (isActiveWindow())
            resumeGrabKeyboard();
        else
            releaseKeyboard();
    }

    return QWidget::eventFilter(_object, _event);
}

void GalleryWidget::updateControls()
{
    updateNavButtons();
    if (const auto currentItem = contentLoader_->currentItem())
    {
        GalleryFrame::Actions menuActions = { GalleryFrame::Copy, GalleryFrame::SaveAs };

        const auto hasMsg = currentItem->msg() != 0;
        if (hasMsg)
        {
            if (const auto sender = currentItem->sender(); !sender.isEmpty())
                controlsWidget_->setAuthorText(Logic::GetFriendlyContainer()->getFriendly(sender));

            const auto t = QDateTime::fromSecsSinceEpoch(currentItem->time());
            const auto dateStr = t.date().toString(u"dd.MM.yyyy");
            const auto timeStr = t.toString(u"hh:mm");

            controlsWidget_->setDateText(QT_TRANSLATE_NOOP("previewer", "%1 at %2").arg(dateStr, timeStr));
            controlsWidget_->setCaption(currentItem->caption());

            menuActions |= GalleryFrame::GoTo;

            if (!Logic::getContactListModel()->isThread(currentItem->aimId()))
            {
                menuActions |= GalleryFrame::Forward;

                if (!Favorites::isFavorites(aimId_))
                    menuActions |= GalleryFrame::SaveToFavorites;
            }
        }
        controlsWidget_->setMenuActions(menuActions);
        controlsWidget_->setAuthorAndDateVisible(hasMsg);

        const auto counterText = [this]() -> QString
        {
            const auto index = contentLoader_->currentIndex();
            const auto total = contentLoader_->totalCount();
            if (index > 0 && total > 0)
                return getInfoText(index, total);
            return {};
        }();
        controlsWidget_->setCounterText(counterText);
    }
}

void GalleryWidget::updateControlsHeight(int _add)
{
    controlWidgetHeight_ = getControlsWidgetHeight() + _add;
    updateControlsFrame();
    update();
}

void GalleryWidget::onPrevClicked()
{
    prev();
}

void GalleryWidget::onNextClicked()
{
    next();
}

void GalleryWidget::onSaveAs()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    if (showVirusToastIfInfected(item))
        return;

    if constexpr (platform::is_linux())
    {
        releaseKeyboard();
        setWindowFlags(windowFlags() & ~Qt::BypassWindowManagerHint);
        imageViewer_->hide();
    }

    if constexpr (platform::is_apple())
        bringToBack();

    QString savePath;

    const auto guard = QPointer(this);

    Utils::saveAs(item->fileName(), [this, item, guard, &savePath](const QString& name, const QString& dir, bool exportAsPng)
    {
        if (!guard)
            return;

        if constexpr (platform::is_linux())
            grabKeyboard();

        const auto destinationFile = QFileInfo(dir, name).absoluteFilePath();

        item->save(destinationFile, exportAsPng);

        savePath = destinationFile;

    }, std::function<void ()>(), false);

    if (!guard)
        return;

    if constexpr (platform::is_linux())
    {
        setWindowFlags(windowFlags() | Qt::BypassWindowManagerHint);
        show();
        imageViewer_->show();
    }

    if constexpr (platform::is_apple())
        showOverAll();

    activateWindow();

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "save" } });
}

void GalleryWidget::onMediaLoaded()
{
    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    delayTimer_->stop();
    progressButton_->hide();
    controlsWidget_->setSaveEnabled(true);
    controlsWidget_->setMenuEnabled(true);

    Ui::ToastManager::instance()->hideToast();
    imageViewer_->show();

    item->showMedia(imageViewer_);

    updateControls();

    controlsWidget_->setZoomVisible(imageViewer_->isZoomSupport());

    const bool virusInfected = item->isMediaVirusInfected() && Features::isAntivirusCheckEnabled();
    controlsWidget_->setZoomInEnabled(!virusInfected);
    controlsWidget_->setZoomOutEnabled(!virusInfected);

    controlsWidget_->show();

    connectContainerEvents();

    if (virusInfected)
        Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("toast", "File is blocked by the antivirus")), getToastPos());
}

void GalleryWidget::onPreviewLoaded()
{
    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    imageViewer_->show();
    Ui::ToastManager::instance()->hideToast();

    item->showPreview(imageViewer_);

    updateControls();

    controlsWidget_->setZoomInEnabled(true);
    controlsWidget_->setZoomOutEnabled(true);

    controlsWidget_->show();

    showProgress();

    connectContainerEvents();
}

void GalleryWidget::onImageLoadingError()
{
    delayTimer_->stop();
    progressButton_->hide();

    controlsWidget_->setSaveEnabled(false);
    controlsWidget_->setMenuEnabled(false);

    auto item = contentLoader_->currentItem();
    if (!item)
        return;

    imageViewer_->hide();
    Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("previewer", "Loading error")), getToastPos());
}

void GalleryWidget::updateNavButtons()
{
    auto navSupported = contentLoader_->navigationSupported();
    controlsWidget_->setNavigationButtonsVisible(navSupported);
    controlsWidget_->setPrevEnabled(hasPrev());
    controlsWidget_->setNextEnabled(hasNext());

    prevButton_->setVisible(hasPrev() && navSupported);
    nextButton_->setVisible(hasNext() && navSupported);
}

void GalleryWidget::onZoomIn()
{
    Q_EMIT closeContextMenu(QPrivateSignal());
    imageViewer_->zoomIn();
}

void GalleryWidget::onZoomOut()
{
    Q_EMIT closeContextMenu(QPrivateSignal());
    imageViewer_->zoomOut();
}

void GalleryWidget::onResetZoom()
{
    Q_EMIT closeContextMenu(QPrivateSignal());
    imageViewer_->resetZoom();
    controlsWidget_->setZoomInEnabled(true);
    controlsWidget_->setZoomOutEnabled(true);
}

void GalleryWidget::onWheelEvent(const QPoint& _delta)
{
    static const auto minDelta = 30;

    const auto delta = _delta.x();

    if (std::abs(delta) < minDelta)
        return;

    if (scrollTimer_->isActive())
        return;

    scrollTimer_->start(scrollTimeoutMsec);

    if (delta < 0 && hasNext())
        next();
    else if (delta > 0 && hasPrev())
        prev();
}

void GalleryWidget::onEscapeClick()
{
    if (!imageViewer_->closeFullscreen())
        closeGallery();

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "cross_btn" } });
}

void GalleryWidget::onSave()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    if (showVirusToastIfInfected(item))
        return;

    QDir downloadDir(Ui::getDownloadPath());
    if (!downloadDir.exists())
        downloadDir.mkpath(qsl("."));

    auto fileName = item->fileName();
    bool exportAsPng = false;
    if (constexpr QStringView webpExt = u".webp"; fileName.endsWith(webpExt, Qt::CaseInsensitive))
    {
        fileName = QStringView(fileName).chopped(webpExt.size()) % u".png";
        exportAsPng = true;
    }

    item->save(downloadDir.filePath(fileName), exportAsPng);

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "save" } });
}

void GalleryWidget::onForward()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    if (showVirusToastIfInfected(item))
        return;

    Data::Quote q;
    q.chatId_ = item->aimId();
    q.senderId_ = item->sender();
    q.text_ = item->link();
    q.msgId_ = item->msg();
    q.time_ = item->time();
    q.senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(q.senderId_);

    escape();

    QTimer::singleShot(0, [quote = std::move(q), isFavorites = Favorites::isFavorites(aimId_)]() mutable { Ui::forwardMessage({ std::move(quote) }, false, QString(), QString(), !isFavorites); });

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "forward" } });
}

void GalleryWidget::onGoToMessage()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    const auto msgId = item->msg();
    if (msgId > 0)
    {
        Utils::InterConnector::instance().openDialog(aimId_, msgId);
        escape();
    }

    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", "go_to_message" } });
}

void GalleryWidget::onCopy()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    if (showVirusToastIfInfected(item))
        return;

    item->copyToClipboard();

    Utils::showCopiedToast();
}

void GalleryWidget::onOpenContact()
{
    const auto item = contentLoader_->currentItem();
    if (!item || !QDateTime::fromSecsSinceEpoch(item->time()).date().isValid())
        return;

    Utils::openDialogOrProfile(item->sender());
    escape();
}

void GalleryWidget::onSaveToFavorites()
{
    const auto item = contentLoader_->currentItem();
    if (!item)
        return;

    Data::Quote q;
    q.chatId_ = item->aimId();
    q.senderId_ = item->sender();
    q.text_ = item->link();
    q.msgId_ = item->msg();
    q.time_ = item->time();
    q.senderFriendly_ = Logic::GetFriendlyContainer()->getFriendly(q.senderId_);

    Favorites::addToFavorites({ q });

    Ui::ToastManager::instance()->showToast(new Favorites::FavoritesToast(1), getToastPos());
}

void GalleryWidget::onDelayTimeout()
{
    delayTimer_->stop();
    controlsWidget_->setSaveEnabled(false);
    controlsWidget_->setMenuEnabled(false);
    controlsWidget_->setZoomVisible(imageViewer_->isZoomSupport());

    auto item = contentLoader_->currentItem();
    if (item && item->isVideo())
    {
        auto buttonSize = progressButton_->sizeHint();
        auto localPos = QPoint((width() - buttonSize.width()) / 2, (height() - controlsWidget_->height() - buttonSize.height()) / 2);

        progressButton_->move(mapToGlobal(localPos));
        progressButton_->startAnimation();
        progressButton_->show();
    }
}

void GalleryWidget::updateZoomButtons()
{
    controlsWidget_->setZoomInEnabled(imageViewer_->canZoomIn());
    controlsWidget_->setZoomOutEnabled(imageViewer_->canZoomOut());
}

void GalleryWidget::onItemSaved(const QString& _path)
{
    Ui::ToastManager::instance()->showToast(new Ui::SavedPathToast(_path), getToastPos());
}

void GalleryWidget::onItemSaveError()
{
    Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("previewer", "Save error")), getToastPos());
}

void GalleryWidget::clicked()
{
    controlsWidget_->collapseCaption();
}

void GalleryWidget::onFullscreenToggled(bool _enabled)
{
#ifdef __APPLE__
    if (!_enabled)
    {
        raise();
        MacSupport::showNSPopUpMenuWindowLevel(this);
    }
#endif
}

void GalleryWidget::onPlayClicked(bool _paused)
{
    Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::fullmediascr_tap_action, { { "chat_type", Utils::chatTypeByAimId(aimId_) },{ "do", _paused ? "stop" : "play" } });
}

void GalleryWidget::onConfigChanged()
{
    const auto antivirusEnabled = Features::isAntivirusCheckEnabled();
    if (std::exchange(antivirusCheckEnabled_, antivirusEnabled) != antivirusEnabled)
        onMediaLoaded();
}

void GalleryWidget::showProgress()
{
    delayTimer_->start(delayTimeoutMsec);
}

bool GalleryWidget::hasPrev() const
{
    return contentLoader_->hasPrev();
}

void GalleryWidget::prev()
{
    im_assert(hasPrev());

    Q_EMIT closeContextMenu(QPrivateSignal());
    showProgress();

    contentLoader_->prev();

    updateControls();
    controlsWidget_->onPrev();
}

bool GalleryWidget::hasNext() const
{
    return contentLoader_->hasNext();
}

void GalleryWidget::next()
{
    im_assert(hasNext());

    Q_EMIT closeContextMenu(QPrivateSignal());
    showProgress();

    contentLoader_->next();

    updateControls();
    controlsWidget_->onNext();
}

void GalleryWidget::updateControlsFrame()
{
    controlsWidget_->setFixedSize(QSize(width(), controlWidgetHeight_));

    auto y = height() - controlsWidget_->height();

    controlsWidget_->move(0, y);
    controlsWidget_->raise();

    static const auto bwidth = Utils::scale_value(navButtonWidth);

    nextButton_->setFixedSize(bwidth, height());
    prevButton_->setFixedSize(bwidth, height());

    nextButton_->move(width() - bwidth, 0);
    prevButton_->move(0, 0);

    nextButton_->raise();
    prevButton_->raise();
}

QSize GalleryWidget::getViewSize()
{
    updateControlsFrame();

    return QSize(width(), height() - controlWidgetHeight_);
}

QPoint GalleryWidget::getToastPos() const
{
    auto localPos = QPoint(width() / 2, height() - Utils::scale_value(180));
    return mapToGlobal(localPos);
}

ContentLoader* GalleryWidget::createGalleryLoader(const Utils::GalleryData& _data)
{
    auto galleryLoader = new GalleryLoader(_data);

    connect(galleryLoader, &GalleryLoader::previewLoaded, this, &GalleryWidget::onPreviewLoaded);
    connect(galleryLoader, &GalleryLoader::mediaLoaded, this, &GalleryWidget::onMediaLoaded);
    connect(galleryLoader, &GalleryLoader::itemUpdated, this, &GalleryWidget::updateControls);
    connect(galleryLoader, &GalleryLoader::contentUpdated, this, &GalleryWidget::updateNavButtons);
    connect(galleryLoader, &GalleryLoader::itemSaved, this, &GalleryWidget::onItemSaved);
    connect(galleryLoader, &GalleryLoader::itemSaveError, this, &GalleryWidget::onItemSaveError);
    connect(galleryLoader, &GalleryLoader::error, this, &GalleryWidget::onImageLoadingError);

    return galleryLoader;
}

ContentLoader* GalleryWidget::createAvatarLoader(const QString& _aimId)
{
    auto avatarLoader = new AvatarLoader(_aimId);

    connect(avatarLoader, &AvatarLoader::mediaLoaded, this, &GalleryWidget::onMediaLoaded);
    connect(avatarLoader, &AvatarLoader::itemSaved, this, &GalleryWidget::onItemSaved);
    connect(avatarLoader, &AvatarLoader::itemSaveError, this, &GalleryWidget::onItemSaveError);

    return avatarLoader;
}

void GalleryWidget::init()
{
    moveToScreen();

    updateControlsFrame();

#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
#else
    showFullScreen();
#endif

    showProgress();

    grabKeyboard();

    grabGesture(Qt::PinchGesture);

    updateControls();

    imageViewer_->setViewSize(getViewSize());
}

void GalleryWidget::bringToBack()
{
#ifdef __APPLE__
    MacSupport::showNSFloatingWindowLevel(this);
    imageViewer_->bringToBack();
#endif
}

void GalleryWidget::showOverAll()
{
#ifdef __APPLE__
    MacSupport::showNSPopUpMenuWindowLevel(this);
    imageViewer_->showOverAll();
#endif
}

void GalleryWidget::connectContainerEvents()
{
    imageViewer_->connectExternalWheelEvent([this](const QPoint& _delta)
    {
        onWheelEvent(_delta);
    });
}

void GalleryWidget::resumeGrabKeyboard()
{
    if (keyboardGrabber() != this)
        grabKeyboard();
}

bool Previewer::GalleryWidget::showVirusToastIfInfected(ContentItem* _item) const
{
    const bool virusInfected = _item->isMediaVirusInfected() && Features::isAntivirusCheckEnabled();
    if (virusInfected)
        Ui::ToastManager::instance()->showToast(new Ui::Toast(QT_TRANSLATE_NOOP("toast", "File is blocked by the antivirus")), getToastPos());
    return virusInfected;
}

NavigationButton::NavigationButton(QWidget *_parent)
    : QWidget(_parent)
{
    setCursor(Qt::PointingHandCursor);
    setMouseTracking(true);
}

NavigationButton::~NavigationButton() = default;

void NavigationButton::setHoveredPixmap(const QPixmap& _pixmap)
{
    hoveredPixmap_ = _pixmap;
}

void NavigationButton::setDefaultPixmap(const QPixmap &_pixmap)
{
    defaultPixmap_ = _pixmap;

    if (activePixmap_.isNull())
        activePixmap_ = defaultPixmap_;
}

void NavigationButton::setFixedSize(int _width, int _height)
{
    QWidget::setFixedSize(_width, _height);
}

void NavigationButton::setVisible(bool _visible)
{
    QWidget::setVisible(_visible);

    activePixmap_ = defaultPixmap_;
}

void NavigationButton::mousePressEvent(QMouseEvent *_event)
{
    _event->accept();
}

void NavigationButton::mouseReleaseEvent(QMouseEvent *_event)
{
    const auto mouseOver = rect().contains(_event->pos());
    if (mouseOver)
        Q_EMIT clicked();

    activePixmap_ = mouseOver ? hoveredPixmap_ : defaultPixmap_;

    update();
}

void NavigationButton::mouseMoveEvent(QMouseEvent *_event)
{
    QWidget::enterEvent(_event);

    activePixmap_ = hoveredPixmap_;

    update();
}

void NavigationButton::leaveEvent(QEvent *_event)
{
    QWidget::leaveEvent(_event);

    activePixmap_ = defaultPixmap_;

    update();
}

void NavigationButton::paintEvent(QPaintEvent *_event)
{
    QPainter p(this);

    static const auto iconSize = Utils::scale_value(navButtonIconSize);

    const auto thisRect = rect();

    const QRect rcButton((thisRect.width() - iconSize.width()) / 2, (thisRect.height() - iconSize.height()) / 2, iconSize.width(), iconSize.height());

    p.drawPixmap(rcButton, activePixmap_);
}

AccessibleGalleryWidget::AccessibleGalleryWidget(QWidget* widget)
    : QAccessibleWidget(widget)
{
    galleryWidget_ = qobject_cast<GalleryWidget*>(widget);

    children_.emplace_back(galleryWidget_->nextButton_);
    children_.emplace_back(galleryWidget_->prevButton_);
    children_.emplace_back(galleryWidget_->controlsWidget_);
    children_.emplace_back(galleryWidget_->imageViewer_);
}

QAccessibleInterface* AccessibleGalleryWidget::child(int index) const
{
    if (index < 0 || index + 1 > static_cast<int>(children_.size()))
        return nullptr;
    return QAccessible::queryAccessibleInterface(children_.at(index));
}

int AccessibleGalleryWidget::indexOfChild(const QAccessibleInterface* child) const
{
    return Utils::indexOf(children_.cbegin(), children_.cend(), child->object());
}
