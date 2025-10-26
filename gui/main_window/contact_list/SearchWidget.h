#pragma once

#include "../../controls/LineEditEx.h"

namespace Ui
{
    class CustomButton;

    namespace TextRendering
    {
        class TextUnit;
    }

    class SearchEdit : public LineEditEx
    {
        Q_OBJECT

    protected:
        void paintEvent(QPaintEvent* _e) override;
        void keyPressEvent(QKeyEvent* _e) override;

    public:
        explicit SearchEdit(QWidget* _parent);
        ~SearchEdit();

        void setPlaceholderTextEx(const QString& _text, bool _drawAnyway = false);
        QString getPlaceholder() const;

    private:
        QString placeHolderText_;
        std::unique_ptr<TextRendering::TextUnit> placeholderTextUnit_;
        bool drawPlaceholderAnyway_;
    };

    class SearchWidget : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:
        void search(const QString&);
        void searchBegin();
        void searchEnd();
        void inputEmpty();
        void enterPressed();
        void upPressed();
        void downPressed();
        void escapePressed();
        void nonActiveButtonPressed();
        void searchIconClicked();
        void activeChanged(bool _isActive);
        void inputCleared();
        void selectFirstInRecents(QPrivateSignal) const;

    public Q_SLOTS:
        void setText(const QString& _text);
        void searchCompleted();
        void clearInput();

    public:
        SearchWidget(QWidget* _parent = nullptr, int _add_hor_space = 0, int _add_ver_space = 0);
        ~SearchWidget();

        QString getText() const;
        bool isEmpty() const;

        void setTabFocus();
        void setFocus(Qt::FocusReason _reason = Qt::MouseFocusReason);
        void clearFocus();
        bool hasFocus() const;

        bool isActive() const;

        void setDefaultPlaceholder();
        void setTempPlaceholderText(const QString& _text);
        void setPlaceholderText(const QString& _text);

        void setEditEventFilter(QObject* _obj);
        void setNeedClear(const bool _clear) { needsClear_ = _clear; }
        void clearSearch(const bool _force);

    protected:
        void enterEvent(QEvent* _event) override;
        void leaveEvent(QEvent* _event) override;
        bool eventFilter(QObject* _obj, QEvent* _ev) override;
        void focusInEvent(QFocusEvent*) override;
        void focusOutEvent(QFocusEvent*) override;
        void keyPressEvent(QKeyEvent* _e) override;

    private:
        void searchStarted();
        void searchChanged(const QString&);
        void focusedOut();
        void focusedIn();
        void onEscapePress();
        void updateClearIconVisibility();

        void setActive(bool _active);

        enum class SetFocusToInput
        {
            No,
            Yes
        };
        void searchCompletedImpl(const SetFocusToInput _setFocus);

    private Q_SLOTS:
        void updateStyle();

    private:
        SearchEdit* searchEdit_;
        bool active_;

        QVBoxLayout *vMainLayout_;
        QHBoxLayout *hSearchLayout_;
        QHBoxLayout *hMainLayout;
        QWidget *searchWidget_;
        QHBoxLayout *horLineLayout_;
        CustomButton *clearIcon_;
        QLabel *searchIcon_;

        QString searchedText_;
        bool needsClear_;

        QString savedPlaceholder_;
    };
}
