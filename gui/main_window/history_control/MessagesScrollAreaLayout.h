#pragma once

#include "history/Message.h"
#include "history/History.h"
#include "../../types/filesharing_meta.h"

namespace hist
{
    class DateInserter;
}

namespace Heads
{
    class HeadContainer;
}

namespace Utils
{
    class OpacityEffect;
    struct FileSharingId;
}

namespace Ui
{
    class MessagesScrollbar;
    class MessagesScrollArea;
    class SmartReplyWidget;

    using highlightsV = std::vector<QString>;

    enum class ScrollOnInsert
    {
        None,
        ScrollToBottomIfNeeded,
        ForceScrollToBottom
    };

    enum class RemoveOthers
    {
        No,
        Yes
    };

    struct InsertHistMessagesParams
    {
        using PositionWidget = std::pair<Logic::MessageKey, std::unique_ptr<QWidget>>;
        using WidgetsList = std::vector<PositionWidget>;

        WidgetsList widgets;

        highlightsV highlights_;

        RemoveOthers removeOthers = RemoveOthers::No;

        std::optional<ScrollOnInsert> scrollOnInsert;
        std::optional<qint64> scrollToMesssageId;
        std::optional<qint64> lastReadMessageId;
        std::optional<qint64> newPlateId;
        hist::scroll_mode_type scrollMode = hist::scroll_mode_type::none;
        bool isBackgroundMode = false;
        bool updateExistingOnly = false;
        bool forceUpdateItems = false;
        bool isThread_ = false;
    };

    class MessagesScrollAreaLayout : public QLayout
    {
        Q_OBJECT;

        enum class SlideOp;

        friend QTextStream& operator <<(QTextStream &lhs, const SlideOp slideOp);

    public:
        using MessageItemVisitor = std::function<bool(Ui::MessageItem*, const bool)>;

        using WidgetVisitor = std::function<bool(QWidget*, const bool)>;

        // the left value is inclusive, the right value is exclusive
        typedef std::pair<int32_t, int32_t> Interval;

        MessagesScrollAreaLayout(
            MessagesScrollArea *scrollArea,
            MessagesScrollbar *messagesScrollbar,
            QWidget *typingWidget,
            hist::DateInserter* dateInserter
        );

        virtual ~MessagesScrollAreaLayout();

        void setGeometry(const QRect &r) override;

        void addItem(QLayoutItem *item) override;

        QLayoutItem *itemAt(int index) const override;

        QLayoutItem *takeAt(int index) override;

        int count() const override;

        QSize sizeHint() const override;

        void invalidate() override;

        QPoint absolute2Viewport(const QPoint absPos) const;

        bool containsWidget(QWidget *widget) const;

        QWidget* getItemByPos(const int32_t pos) const;

        QWidget* getItemByKey(const Logic::MessageKey &key) const;

        QWidget* extractItemByKey(const Logic::MessageKey &key); // caller takes ownership

        QWidget* getPrevItem(QWidget* _w) const;

        int32_t getItemsCount() const;

        int32_t getItemsHeight() const;

        Logic::MessageKeyVector getItemsKeys() const;

        int32_t getViewportAbsY() const;

        Interval getViewportScrollBounds() const;

        int32_t getViewportHeight() const;

        QVector<Logic::MessageKey> getWidgetsOverBottomOffset(const int32_t offset) const;
        QVector<Logic::MessageKey> getWidgetsUnderBottomOffset(const int32_t offset) const;

        void insertWidgets(InsertHistMessagesParams&& _params);

        void removeAllExcluded(const InsertHistMessagesParams& _params);

        bool hasItemsOfType(Logic::ControlType _type) const;

        void removeItemsByType(Logic::ControlType _type);

        bool removeItemAtEnd(Logic::ControlType _type);

        void lock();
        void unlock();

        void removeWidget(QWidget *widget);
        void removeWidgets(const std::vector<QWidget*>& widgets);

        void setViewportByOffset(const int32_t bottomOffset);

        int32_t shiftViewportAbsY(const int32_t delta);

        void enableViewportShifting(bool enable);

        void resetShiftingParams();

        void updateItemKey(const Logic::MessageKey &key);

        void scrollTo(const Logic::MessageKey &key, hist::scroll_mode_type _scrollMode);

        void enumerateWidgets(const WidgetVisitor& visitor, const bool reversed);

        bool isViewportAtBottom() const;

        void resumeVisibleItems();

        void updateDistanceForViewportItems();

        void suspendVisibleItems();

        void updateItemsWidth();

        void readVisibleItems();

        Logic::MessageKey firstAvailableToSelect() const;

        Logic::MessageKey nextAvailableToSelect(qint64 _current) const;

        Logic::MessageKey prevAvailableToSelect(qint64 _current) const;

        QPoint viewport2Absolute(QPoint viewportPos) const;

        void updateBounds();

        void updateScrollbar();

        void updateItemsProps();

        bool isInitState() const noexcept;
        void resetInitState() noexcept;

        bool hasItems() const noexcept { return !LayoutItems_.empty(); }

        bool eventFilter(QObject* watcher, QEvent* e) override;

        int32_t getBottomWidgetsHeight() const;
        int32_t getSmartReplyWidgetHeight() const;
        int32_t getSmartReplyButtonHeight() const;
        int32_t getTypingWidgetHeight() const;
        Interval getItemsAbsBounds() const;

        void updateItemsGeometry();

        void setHeadContainer(Heads::HeadContainer*);

        void checkVisibilityForRead();

        void setSmartreplyWidget(SmartReplyWidget* _widget);
        void showSmartReplyWidgetAnimated();
        void hideSmartReplyWidgetAnimated();

        void setSmartrepliesSemitransparent(bool _semi);

        void setSmartreplyButton(QWidget* _button);
        void notifyQuotesResize();

    private:

        class ItemInfo
        {
        public:
            ItemInfo(QWidget *widget, const Logic::MessageKey &key);

            QWidget *Widget_;

            QRect AbsGeometry_;

            Logic::MessageKey Key_;
            Data::MessageBuddy Buddy_;

            bool IsGeometrySet_;

            bool IsHovered_;

            bool IsActive_;

            QRect visibleRect_;

            bool isVisibleEnoughForRead_;
        };

        using ItemInfoUptr = std::unique_ptr<ItemInfo>;
        using ItemsInfo = std::deque<ItemInfoUptr>;
        using ItemsInfoIter = ItemsInfo::iterator;

        std::unordered_set<QWidget*> Widgets_;

        ItemsInfo LayoutItems_;

        MessagesScrollbar *Scrollbar_;

        MessagesScrollArea *ScrollArea_;

        QWidget* TypingWidget_;
        SmartReplyWidget* smartreplyWidget_;
        QWidget* smartreplyButton_;

        hist::DateInserter* dateInserter_;

        QRect LayoutRect_;

        QSize ViewportSize_;

        int32_t ViewportAbsY_;

        bool IsDirty_;

        hist::SimpleRecursiveLock updatesLocker_;

        bool ShiftingViewportEnabled_;

        struct ShiftingParams
        {
            std::optional<qint64> messageId;
            hist::scroll_mode_type type = hist::scroll_mode_type::none;
            int counter = 0;
            highlightsV highlight_;
        };

        ShiftingParams ShiftingParams_;

        bool applyShiftingParams();
        void setViewportAbsYImpl(int32_t _y);

        QRect absolute2Viewport(QRect absolute) const;

        void applyItemsGeometry();

        void applyBottomWidgetsGeometry();
        void applyTypingWidgetGeometry();
        void applySmartReplyWidgetGeometry();
        void applySmartReplyButtonGeometry();

        QRect calculateInsertionRect(const ItemsInfoIter &itemInfoIter, Out SlideOp &slideOp);

        void debugValidateGeometry();

        void removeDatesImpl();
        std::vector<Logic::MessageKey> getKeysForDates() const;
        InsertHistMessagesParams::WidgetsList makeDates(const std::vector<Logic::MessageKey>& _keys) const;

        std::optional<Logic::MessageKey> getKeyForReplies() const;
        InsertHistMessagesParams::PositionWidget makeReplies(const Logic::MessageKey& _keys) const;

        int insertWidgetsImpl(InsertHistMessagesParams& _params);

        void dumpGeometry(const QString &notes);

        int32_t evalViewportAbsMiddleY() const;

        QRect evalViewportAbsRect() const;

        int32_t getRelY(const int32_t y) const;

        ItemsInfoIter insertItem(QWidget *widget, const Logic::MessageKey &key);

        void onItemActivityChanged(QWidget *widget, const bool isActive);
        void onItemVisibleRectChanged(QWidget* widget, const QRect& _visibleRect);

        void onItemRead(QWidget *widget, const bool _isVisible);

        void onItemDistanseToViewPortChanged(QWidget *widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect);

        bool setViewportAbsY(const int32_t absY);

        void simulateMouseEvents(ItemInfo &itemInfo, const QRect &scrollAreaWidgetGeometry, const QPoint &globalMousePos, const QPoint &scrollAreaMousePos);

        bool slideItemsApart(const ItemsInfoIter &changedItemIter, const int slideY, const SlideOp slideOp);

        void moveViewportToBottom();

        int getWidthForItem() const;

        int getXForItem() const;

        void updateItemsPropsDirect();
        void updateItemsPropsReverse();

        QRect getItemVisibleRect(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const;
        bool isVisibleEnoughForRead(const QWidget *_widget, const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) const;
        void suspendVisibleItem(const ItemInfoUptr& _item);

        enum class SmartreplyAnimType
        {
            invalid,
            show,
            hide
        };
        void animateSmartReplyWidget(const SmartreplyAnimType _type);
        bool isSmartreplyVisible() const;
        void initSmartreplyOpacity();
        void initSmartreplyButtonOpacity();

        enum class SuspendAfterExtract
        {
            no,
            yes
        };
        void extractItemImpl(QWidget *_widget, SuspendAfterExtract _mode = SuspendAfterExtract::yes);

        std::list<QWidget*> ScrollingItems_;

        /// observe to scrolling item position
        void moveViewportUpByScrollingItems();

        bool isInitState_; // we have no user action yet

        QTimer resetScrollActivityTimer_;
        bool scrollActivityFlag_;

        Heads::HeadContainer* heads_;

        QVariantAnimation* smartreplyAnim_;
        Utils::OpacityEffect* smartreplyOpacity_;
        Utils::OpacityEffect* smartreplyButtonOpacity_;

        int bottomOverflow_ = 0;
        bool quoteHeightChangeWaited_ = false;

    public:
        size_t widgetsCount() const noexcept;

private Q_SLOTS:
        void onDeactivateScrollingItems(QObject* obj);
        /// void onDestroyItemAndAlign(QObject* obj);

        void onButtonDownClicked();

        void onMoveViewportUpByScrollingItem(QWidget* widget);

Q_SIGNALS:
        void updateHistoryPosition(int32_t position, int32_t offset);
        void recreateAvatarRect();
        void moveViewportUpByScrollingItem(QWidget*);
        void itemRead(const qint64 _id, const bool _visible);
    };
}
