#pragma once

#include "../../controls/CustomButton.h"
#include "styles/StyledWidget.h"

namespace Ui
{
    class SearchWidget;
    enum class LeftPanelState;
    class ContactAvatarWidget;

    namespace TextRendering
    {
        class TextUnit;
    }

    class HeaderTitleBar;

    class OverlayTopWidget : public QWidget
    {
        Q_OBJECT;
    public:
        explicit OverlayTopWidget(HeaderTitleBar* _parent = nullptr);
        ~OverlayTopWidget();

    protected:
        void paintEvent(QPaintEvent* _event) override;

    private:
        HeaderTitleBar * parent_;

        friend class HeaderTitleBar;
    };

    class HeaderTitleBarButton : public CustomButton
    {
        Q_OBJECT
    public:
        explicit HeaderTitleBarButton(QWidget* _parent = nullptr);
        ~HeaderTitleBarButton();
        void setBadgeText(const QString& _text);
        QString getBadgeText() const;

        void setPersistent(bool _value) noexcept;
        bool isPersistent() const noexcept;

        void drawBadge(QPainter& _p) const;

        void setVisibility(bool _value);
        bool getVisibility() const noexcept;

    private:
        bool visibility_;
        QString badgeText_;
        bool isPersistent_; // don't hide in compact mode
        std::unique_ptr<TextRendering::TextUnit> badgeTextUnit_;
    };

    class EmailTitlebarButton : public HeaderTitleBarButton
    {
        Q_OBJECT
    public:
        explicit EmailTitlebarButton(QWidget* _parent = nullptr);
        ~EmailTitlebarButton();

    private Q_SLOTS:
        void mailStatus(const QString&, unsigned, bool);
        void openMailBox();

    private:
        QString email_;
    };

    class HeaderTitleBar : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void arrowClicked();

    public:
        explicit HeaderTitleBar(QWidget* _parent = nullptr);
        ~HeaderTitleBar();

        void setTitle(const QString& _title);
        void setSubTitle(const QString& _subtitle);
        void addButtonToLeft(HeaderTitleBarButton* _button);
        void addButtonToRight(HeaderTitleBarButton* _button);
        void addCentralWidget(QWidget* _widget);

        void setButtonSize(QSize _size);

        void setCompactMode(bool _value);
        void refresh();

        void setArrowVisible(bool _visible);
        void setTitleVisible(bool _visible);
        void setCentralWidgetVisible(bool _visible);

        void setTitleLeftOffset(int _offset);

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        void enterEvent(QEvent* _event) override;

    private:
        void addButtonImpl(HeaderTitleBarButton* _button);
        void updateSpacers();
        void updateCursor();

    private:
        QGridLayout* mainLayout_;
        QHBoxLayout* leftLayout_;
        QHBoxLayout* centerLayout_;
        QHBoxLayout* rightLayout_;
        QWidget* leftWidget_;
        QWidget* centerWidget_;
        QWidget* rightWidget_;
        QSpacerItem* leftSpacer_;
        QSpacerItem* rightSpacer_;
        OverlayTopWidget* overlayWidget_;
        std::vector<QPointer<HeaderTitleBarButton>> buttons_;
        bool isCompactMode_;
        QString title_;
        std::unique_ptr<TextRendering::TextUnit> titleTextUnit_;
        QString subtitle_;
        std::unique_ptr<TextRendering::TextUnit> subtitleTextUnit_;
        QSize buttonSize_;
        bool arrowVisible_;
        bool titleVisible_;
        QPoint clicked_;
        int titleLeftOffset_;

        friend class OverlayTopWidget;
    };

    //----------------------------------------------------------------------
    class TopPanelHeader : public QWidget
    {
        Q_OBJECT

    public:
        explicit TopPanelHeader(QWidget* _parent);

        LeftPanelState getState() const;
        virtual void setState(const LeftPanelState _state);

    protected:
        LeftPanelState state_;
    };

    //----------------------------------------------------------------------
    class SearchHeader : public TopPanelHeader
    {
        Q_OBJECT

    Q_SIGNALS:
        void cancelClicked();

    public:
        SearchHeader(QWidget* _parent, const QString& _text);

    private:
        HeaderTitleBar * titleBar_;
    };

    //----------------------------------------------------------------------
    class UnknownsHeader : public TopPanelHeader
    {
        Q_OBJECT

    Q_SIGNALS:
        void backClicked();

    private Q_SLOTS:
        void deleteAllClicked();

    public:
        UnknownsHeader(QWidget* _parent);

        void setState(const LeftPanelState _state) override;

    private:
        CustomButton* backBtn_;
        QPushButton* delAllBtn_;
        QWidget* spacer_;
        QWidget* leftSpacer_;
        QWidget* rightSpacer_;
    };

    //----------------------------------------------------------------------
    class RecentsHeader : public TopPanelHeader
    {
        Q_OBJECT

    Q_SIGNALS:
        void needSwitchToRecents();
        void pencilClicked();

    private Q_SLOTS:
        void titleButtonsUpdated();
        void onPencilClicked();

    public:
        explicit RecentsHeader(QWidget* _parent);

        void setState(const LeftPanelState _state) override;

        void addButtonToLeft(HeaderTitleBarButton* _button);
        void addButtonToRight(HeaderTitleBarButton* _button);

        void refresh();
        void updateTitle();

    private:
        HeaderTitleBar* titleBar_;
        HeaderTitleBarButton* pencil_;
        ContactAvatarWidget* statusWidget_;
    };

    //----------------------------------------------------------------------
    class TopPanelWidget : public TopPanelHeader
    {
        Q_OBJECT

    Q_SIGNALS:
        void back();
        void searchCancelled();
        void needSwitchToRecents();

    private Q_SLOTS:
        void onPencilClicked();
        void searchActiveChanged(const bool _active);
        void searchEscapePressed();

    public:

        enum class Regime
        {
            Invalid,
            Recents,
            Unknowns,
            Search
        };

        TopPanelWidget(QWidget* _showStatus);

        void setState(const LeftPanelState _state) override;
        void setRegime(const Regime _regime);
        Regime getRegime() const;

        SearchWidget* getSearchWidget() const noexcept;
        RecentsHeader* getRecentsHeader() const noexcept;

    protected:
        virtual void paintEvent(QPaintEvent* _e) override;

    private:
        bool isSearchRegime(const Regime _regime);
        void endSearch();
        void updateSearchWidgetVisibility();
        bool isSearchWidgetVisible();
        void updateHeader();

        QStackedWidget* stackWidget_;
        RecentsHeader* recentsHeader_;
        UnknownsHeader* unknownsHeader_;
        SearchHeader* searchHeader_;

        SearchWidget* searchWidget_;

        Regime regime_;
    };
}
