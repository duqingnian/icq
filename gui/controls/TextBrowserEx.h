#pragma once

#include "namespaces.h"
#include "styles/StyleVariable.h"
#include "styles/ThemeColor.h"

namespace Ui {

// http://doc.qt.io/qt-5/qtextdocument.html#documentMargin-prop
constexpr int QTEXTDOCUMENT_DEFAULT_MARGIN = 4;

class TextBrowserEx: public QTextBrowser
{
    Q_OBJECT

public:
    struct Options
    {
        enum class LinksUnderline
        {
            NoUnderline,
            AlwaysUnderline,
            ThemeDependent
        };
        Styling::StyleVariable linkColor_;
        Styling::StyleVariable textColor_;
        Styling::StyleVariable backgroundColor_;
        QFont font_;
        bool borderless_ = true;
        bool openExternalLinks_ = true;
        qreal documentMargin_ = 0.;
        bool useDocumentMargin_ = false;
        QMargins bodyMargins_;
        LinksUnderline linksUnderline_ = LinksUnderline::AlwaysUnderline;

        QMargins getMargins() const;

        int leftBodyMargin() const { return bodyMargins_.left() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.left() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int rightBodyMargin() const { return bodyMargins_.right() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.right() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int bottomBodyMargin() const { return bodyMargins_.bottom() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.bottom() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }
        int topBodyMargin() const { return bodyMargins_.top() > QTEXTDOCUMENT_DEFAULT_MARGIN ? (bodyMargins_.top() - QTEXTDOCUMENT_DEFAULT_MARGIN) : 0; }

        Options();
    };

    TextBrowserEx(const Options& _options, QWidget* _parent =  nullptr);

    const Options& getOptions() const;
    void setLineSpacing(int lineSpacing);

    bool event(QEvent* _event) override;

    void setHtmlSource(const QString& _html);

private:
    void updateStyle();

private:
    Options options_;
    Styling::ThemeChecker themeChecker_;
    QString html_;
};

namespace TextBrowserExUtils
{
    int setAppropriateHeight(TextBrowserEx& _textBrowser);
}

}
