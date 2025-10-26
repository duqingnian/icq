#pragma once

#include <QWidget>

#include "../controls/TextUnit.h"
#include "../types/filesharing_download_result.h"

namespace Ui
{

//////////////////////////////////////////////////////////////////////////
// ToastBase
//////////////////////////////////////////////////////////////////////////

class ToastBase : public QWidget
{
    Q_OBJECT
public:
    enum class MoveDirection
    {
        TopToBottom,
        BottomToTop
    };
    explicit ToastBase(QWidget *parent = nullptr);

    void showAt(const QPoint& _center, bool _onTop = false);
    virtual bool keepRows() const { return keepRows_;}
    virtual bool sameSavePath(const ToastBase* _other) const { return false;}
    virtual QString getPath() const { return QString(); }
    void setDirection(MoveDirection _dir);
    void setVisibilityDuration(std::chrono::milliseconds _duration);
    void setUseMainWindowShift(bool _enable);
    void setBackgroundColor(const QColor& _color);
    void enableMoveAnimation(bool _enable);
    void enableMultiScreenShowing(bool _enable);
    void setKeepRows(bool _keepRows);

    bool isMultiScreenShowing() const { return isMultiScreenShowingEnabled_; }

Q_SIGNALS:
    void appeared();

protected:
    void paintEvent(QPaintEvent* _event) override;
    void enterEvent(QEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

    bool eventFilter(QObject* _object, QEvent* _event) override;

    virtual void drawContent(QPainter& _p) = 0;

    void handleParentResize(const QSize& _newSize, const QSize& _oldSize);
    void handleMouseLeave();
    virtual void updateSize() {}

private Q_SLOTS:
    void startHide();

private:
    QTimer hideTimer_;
    QTimer startHideTimer_;
    QColor bgColor_;
    MoveDirection direction_;

    QVariantAnimation* opacityAnimation_;
    QVariantAnimation* moveAnimation_;

    QGraphicsOpacityEffect* opacity_;

    bool useMainWindowShift_ = false;
    bool isMoveAnimationEnabled_ = true;
    bool isMultiScreenShowingEnabled_ = false;
    bool keepRows_ = false;
};

//////////////////////////////////////////////////////////////////////////
// ToastManager
//////////////////////////////////////////////////////////////////////////

class ToastManager : public QObject
{
    Q_OBJECT
public:
    static ToastManager* instance();

    void showToast(ToastBase* _toast, QPoint _pos);

    void hideToast();
    void moveToast();

    void raiseBottomToastIfPresent();

protected:
    bool eventFilter(QObject* _object, QEvent* _event) override;

private Q_SLOTS:
    void clearMultiScreenToastsIfNeeded();

private:
    ToastManager() = default;
    ToastManager(const ToastManager& _other) = delete;

    QPointer<ToastBase> bottomToast_;
    QPointer<ToastBase> topToast_;
    std::vector<QPointer<ToastBase>> multiScreenToasts_;
};

//////////////////////////////////////////////////////////////////////////
// Toast
//////////////////////////////////////////////////////////////////////////

class Toast : public ToastBase // simple toast with text
{
public:
    Toast(const QString& _text, QWidget* _parent = nullptr, int _maxLineCount = 1);

    void setText(const QString& _text, int _maxLineCount = 1);

protected:
    void drawContent(QPainter& _p) override;
    void updateSize() override;

private:
    Ui::TextRendering::TextUnitPtr textUnit_;
};

//////////////////////////////////////////////////////////////////////////
// SavedPathToast
//////////////////////////////////////////////////////////////////////////

class SavedPathToast_p;

class SavedPathToast : public ToastBase
{
public:
    SavedPathToast(const QString& _path, QWidget* _parent = nullptr);

protected:
    void drawContent(QPainter& _p) override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;

private:
    std::unique_ptr<SavedPathToast_p> d;
};

//////////////////////////////////////////////////////////////////////////
// DownloadFinishedToast
//////////////////////////////////////////////////////////////////////////

class DownloadFinishedToast_p;

class DownloadFinishedToast : public ToastBase
{
public:
    DownloadFinishedToast(const QString& _chatAimId, const Data::FileSharingDownloadResult& _result, QWidget* _parent = nullptr);
    ~DownloadFinishedToast();


    QString getPath() const override;
    bool keepRows() const override { return true;}
    bool sameSavePath(const ToastBase* _other) const override;
protected:
    void drawContent(QPainter& _p) override;

    void mouseMoveEvent(QMouseEvent* _event) override;
    void mousePressEvent(QMouseEvent* _event) override;
    void mouseReleaseEvent(QMouseEvent* _event) override;
    void leaveEvent(QEvent* _event) override;
    void updateSize() override;

private:
    void compose(bool _isMinimal, const int maxWidth, const QFontMetrics fm, int fileWidth, int pathWidth, int _totalWidth);
private:
    std::unique_ptr<DownloadFinishedToast_p> d;
    int xOffset_;
    int yOffset_;
    int fileWidth_;
    int pathWidth_;
    int totalWidth_;
    QString chatAimId_;
};

}

namespace Utils
{
    void showToastOverMainWindow(const QString& _text, int _bottomOffset, int _maxLineCount = 1); // show text toast over main window

    //! Show text toast over video window, taking into account video contols panel presence
    void showToastOverVideoWindow(const QString& _text, int _maxLineCount = 1);
    void hideVideoWindowToast();

    void showToastOverContactDialog(Ui::ToastBase* _toast);
    void showToastOverMainWindow(Ui::ToastBase* _toast);
    void showTextToastOverContactDialog(const QString& _text, int _maxLineCount = 1); // show text toast over contactDialog
    void showDownloadToast(const QString& _chatAimId, const Data::FileSharingDownloadResult& _result);
    void showCopiedToast(std::optional<std::chrono::milliseconds> _visibilityDuration = std::nullopt);

    void showMultiScreenToast(const QString& _text, int _maxLineCount = 1);
    void showWebDownloadToast(const QString& _filename, bool _downloaded);

    inline constexpr std::chrono::milliseconds defaultCopiedToastDuration() noexcept { return std::chrono::seconds(1); }

    int defaultToastVerOffset() noexcept;
}
