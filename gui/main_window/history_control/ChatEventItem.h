#pragma once

#include "HistoryControlPageItem.h"

namespace HistoryControl
{
    typedef std::shared_ptr<class ChatEventInfo> ChatEventInfoSptr;
}

namespace Ui
{
    namespace TextRendering
    {
        class TextUnit;
    }

    class RoundButton;

    class ChatEventItem : public HistoryControlPageItem
    {
        Q_OBJECT

    public:
        ChatEventItem(const ::HistoryControl::ChatEventInfoSptr& _eventInfo, const qint64 _id, const qint64 _prevId);
        ChatEventItem(QWidget* _parent, const ::HistoryControl::ChatEventInfoSptr& eventInfo, const qint64 _id, const qint64 _prevId);
        ChatEventItem(QWidget* _parent, std::unique_ptr<TextRendering::TextUnit> _textUnit);

        ~ChatEventItem();

        void clearSelection(bool) override;

        QString formatRecentsText() const override;

        MediaType getMediaType(MediaRequestMode _mode = MediaRequestMode::Chat) const override;

        QSize sizeHint() const override;

        void setLastStatus(LastStatus _lastStatus) override;

        qint64 getId() const override { return id_; }
        qint64 getPrevId() const override { return prevId_; }

        void setQuoteSelection() override {}

        void updateFonts() override;

        void updateSize() override;

        bool isOutgoing() const override;
        int32_t getTime() const override;

        int bottomOffset() const override;

        static Styling::ThemeColorKey getTextColor(const QString& _contact);
        static Styling::ThemeColorKey getLinkColor(const QString& _contact);
        static QFont getTextFont(int _size = -1);
        static QFont getTextFontBold(int _size = -1);

    private Q_SLOTS:
        void onChatInfo();
        void modChatAboutResult(qint64, int);
        void avatarChanged(const QString&);
        void leftButtonClicked();
        void rightButtonClicked();
        void dlgStateChanged(const Data::DlgState&);
        void ignoreListChanged();

    protected:
        void paintEvent(QPaintEvent* _event) override;
        void resizeEvent(QResizeEvent* _event) override;
        void mousePressEvent(QMouseEvent* _event) override;
        void mouseMoveEvent(QMouseEvent* _event) override;
        void mouseReleaseEvent(QMouseEvent* _event) override;

        bool canBeThreadParent() const override { return false; }
        bool supportsOverlays() const override { return false; }

    private:
        QRect BubbleRect_;

        TextRendering::TextUnitPtr TextWidget_;

        const ::HistoryControl::ChatEventInfoSptr EventInfo_;

        int height_ = 0;

        qint64 id_ = -1;
        qint64 prevId_ = -1;

        QPoint pressPos_;

        QWidget* textPlaceholder_ = nullptr;
        QWidget* buttonsWidget_ = nullptr;
        RoundButton* btnLeft_ = nullptr;
        RoundButton* btnRight_ = nullptr;

        bool buttonsVisible_ = false;
        bool leftButtonVisible_ = false;
        bool rightButtonVisible_ = false;

        qint64 modChatAboutSeq_ = -1;

    private:
        void init();
        void initTextWidget();

        int32_t evaluateTextWidth(const int32_t _widgetWidth);
        void updateSize(const QSize& _size);

        void initButtons();

        bool hasButtons() const;

        void updateButtons();

        int textFontSize() const;

        int buttonsfontSize() const;

        int getButtonsHeight() const;

        int lineSpacing() const;

        TextRendering::HorAligment textAlignment() const;

        bool membersLinksEnabled() const;
    };

}
