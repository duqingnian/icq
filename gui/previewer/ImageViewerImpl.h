#pragma once

#include "ImageViewerWidget.h"

namespace Previewer
{
    class AbstractViewer
        : public QObject
    {
        Q_OBJECT

    Q_SIGNALS:
        void mouseWheelEvent(const QPoint& _delta);
        void doubleClicked();
        void rightClicked();
        void loaded();
        void fullscreenToggled(bool _enabled);
        void playClicked(bool _paused);

    public:
        virtual ~AbstractViewer();

        bool canScroll() const;

        QRect rect() const;

        virtual void scale(double _newScaleFactor, QPoint _anchor = QPoint());
        virtual bool scaleBy(double _newScaleFactor, QPoint _anchor = QPoint()) { return false; }
        virtual void move(const QPoint& _offset);

        void paint(QPainter& _painter);

        virtual QWidget* getParentForContextMenu() const = 0;

        double getPreferredScaleFactor() const;
        double getScaleFactor() const;

        double actualScaleFactor() const;

        virtual bool isZoomSupport() const;
        virtual bool closeFullscreen();

        virtual void hide() {}
        virtual void show() {}

        virtual void bringToBack() {}
        virtual void showOverAll() {}

    protected:
        explicit AbstractViewer(const QSize& _viewportSize, QWidget* _parent);

        void init(const QSize& _imageSize, const QSize &_viewport, const int _minSize);

    protected Q_SLOTS:
        void repaint();

    protected:
        virtual void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) = 0;
        virtual void doResize(const QRect& _source, const QRect& _target);

        double getPreferredScaleFactor(const QSize& _imageSize, const int _minSize) const;

        void fixBounds(const QSize& _bounds, QRect& _child);

        QRect getTargetRect(double _scaleFactor) const;

        bool canScroll_;

        QSize imageSize_;

        QRect viewportRect_;

        QRect targetRect_;
        QRect fragment_;

        QPoint offset_;

        QRect initialViewport_;

        double preferredScaleFactor_;
        double scaleFactor_;
    };


    class GifViewer : public AbstractViewer
    {
        Q_OBJECT
    public:
        static std::unique_ptr<AbstractViewer> create(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

        QWidget* getParentForContextMenu() const override;

    private:
        GifViewer(const QString& _fileName, const QSize& _viewportSize, QWidget* _parent);

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QMovie gif_;
    };

    class JpegPngViewer : public AbstractViewer
    {
    public:
        static std::unique_ptr<AbstractViewer> create(const QPixmap& _image,
                                                      const QSize& _viewportSize,
                                                      QWidget* _parent,
                                                      const QSize &_initialViewport,
                                                      const bool _isVideoPreview = false);

        virtual void scale(double _newScaleFactor, QPoint _anchor = QPoint()) override;
        virtual bool scaleBy(double _scaleFactorDiff, QPoint _anchor = QPoint()) override;
        virtual void move(const QPoint& _offset) override;
        QWidget* getParentForContextMenu() const override;

    private:
        JpegPngViewer(const QPixmap& _image,
                      const QSize& _viewportSize,
                      QWidget* _parent,
                      const QSize& _initialViewport);

        void constrainOffset();

        void doPaint(QPainter& _painter, const QRect& _source, const QRect& _target) override;

    private:
        QPoint additionalOffset_;
        QPixmap originalImage_;
        bool smoothUpdate_;
        QTimer smoothUpdateTimer_;
    };
}
