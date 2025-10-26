#pragma once

#include "GenericBlock.h"
#include "QuoteBlockLayout.h"
#include "../../../types/message.h"

#include "../../../controls/TextUnit.h"

UI_NS_BEGIN

class PictureWidget;
class ContactAvatarWidget;

UI_NS_END

UI_COMPLEX_MESSAGE_NS_BEGIN

enum class BlockSelectionType;

class QuoteBlock final : public GenericBlock
{
    friend class QuoteBlockLayout;

    Q_OBJECT

public:
    QuoteBlock(ComplexMessageItem* _parent, const Data::Quote& _quote);

    ~QuoteBlock() override;

    void clearSelection() override;
    void releaseSelection() override;

    IItemBlockLayout* getBlockLayout() const override;

    bool updateFriendly(const QString& _aimId, const QString& _friendly) override;

    Data::FString getSelectedText(const bool _isFullSelect = false, const TextDestination _dest = TextDestination::selection) const override;

    bool isSelected() const override;

    bool isAllSelected() const override;

    void selectByPos(const QPoint& from, const QPoint& to, bool topToBottom) override;

    void selectAll() override;

    void onVisibleRectChanged(const QRect& _visibleRect) override;

    void onSelectionStateChanged(const bool isSelected) override;

    void onDistanceToViewportChanged(const QRect& _widgetAbsGeometry, const QRect& _viewportVisibilityAbsRect) override;

    QRect setBlockGeometry(const QRect &ltr) override;

    void onActivityChanged(const bool isActive) override;

    bool replaceBlockWithSourceText(IItemBlock *block) override;

    bool removeBlock(IItemBlock *block) override;

    bool isSharingEnabled() const override;

    bool containsSharingBlock() const override;

    QString getTextForCopy() const override;

    Data::FString getSourceText() const override;

    QString formatRecentsText() const override;

    bool standaloneText() const override;

    bool isSimple() const override { return false; }

    Data::Quote getQuote() const override;

    bool needFormatQuote() const override { return false; }

    IItemBlock* findBlockUnder(const QPoint &pos) const override;

    void addBlock(GenericBlock* block);

    void setReplyBlock(GenericBlock* block);

    ContentType getContentType() const override { return ContentType::Quote; }

    void setMessagesCountAndIndex(int count, int index);

    int desiredWidth(int _width = 0) const override;

    Data::LinkInfo linkAtPos(const QPoint& pos) const override;

    bool isOverLink(const QPoint& _mousePosGlobal) const override;

    QStringList messageLinks() const override;

    bool clicked(const QPoint& _p) override;
    void doubleClicked(const QPoint& _p, std::function<void(bool)> _callback = std::function<void(bool)>()) override;
    bool pressed(const QPoint& _p) override;

    QString getSenderAimid() const override;

    bool isNeedCheckTimeShift() const override;

    void highlight(const highlightsV& _hl) override;
    void removeHighlight() override;
    Data::FilesPlaceholderMap getFilePlaceholders() override;

    bool containsText() const;

    int getMaxWidth() const override;

    int effectiveBlockWidth() const override;

    void markTrustRequired() override;
    bool isBlockedFileSharing() const override;

    bool needToExpand() const override;

protected:
    void drawBlock(QPainter &p, const QRect& _rect, const QColor& _quoteColor) override;

    void initialize() override;

    void mouseMoveEvent(QMouseEvent* _e) override;

private:
    bool quoteOnly() const;
    bool isQuoteInteractive() const;
    QString getQuoteHeader() const;
    QColor getSenderColor() const;
    int32_t getLeftOffset() const;
    int32_t getLeftAdditional() const;
    void initSenderName();

    void updateFonts() override;

private Q_SLOTS:
    void blockClicked();

Q_SIGNALS:
    void observeToSize();

public:
    // NOTE: Debug purposes only
    const std::vector<GenericBlock *>& getBlocks() const;

private:

    Data::Quote Quote_;

    QuoteBlockLayout *Layout_;

    std::vector<GenericBlock*> Blocks_;

    TextRendering::TextUnitPtr senderLabel_;
    int desiredSenderWidth_;

    QRect Geometry_;

    BlockSelectionType Selection_;

    ComplexMessageItem* Parent_;

    GenericBlock* ReplyBlock_;

    int MessagesCount_;

    int MessageIndex_;

    QRect hoverRect_;
    QPainterPath hoverClipPath_;
};

UI_COMPLEX_MESSAGE_NS_END
