#pragma once
#include "../controls/TextEmojiWidget.h"

namespace Ui
{
    template<typename __Base>
    class ShadowedWidget : public __Base
    {
    public:
        ShadowedWidget(QWidget* _parent = NULL, int _tailLen = 30, double _alphaFrom = 1, double _alphaTo = 0);
        virtual ~ShadowedWidget();

    protected:
        void paintEvent(QPaintEvent*);
        void resizeEvent(QResizeEvent*);

    private:
        //QLinearGradient linearGradient_;
        int tailLenPx_;
    };

    class NameAndStatusWidget : public QWidget
    {
        Q_OBJECT
    public:
        NameAndStatusWidget(QWidget* _parent, int _nameBaseline, int _statusBaseline, int maxAvalibleWidth);
        virtual ~NameAndStatusWidget();

        void setName  (const char* _name);
        void setStatus(const char* _status);

        void setNameProperty(const char* _propName, bool _val);
        void setStatusProperty(const char* _propName, bool _val);

    private:
        TextEmojiWidget* name_;
        TextEmojiWidget* status_;
    };
}
