#include "stdafx.h"
#include "GeneralSettingsWidget.h"

#include "../common.shared/config/config.h"

#include "../MainWindow.h"
#include "../../gui_settings.h"
#include "../../controls/GeneralCreator.h"
#include "../../controls/SimpleListWidget.h"
#include "../../controls/RadioTextRow.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../controls/TransparentScrollBar.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/features.h"
#include "../../styles/ThemeParameters.h"
#include "../../statuses/StatusUtils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initShortcuts(ShortcutsSettings* _parent)
{
    auto scrollArea = CreateScrollAreaAndSetTrScrollBarV(_parent);
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(ql1s("QWidget{border: none; background-color: transparent;}"));

    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    Utils::ApplyStyle( mainWidget, Styling::getParameters().getGeneralSettingsQss());
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = Utils::emptyVLayout(mainWidget);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(0, Utils::scale_value(36), 0, 0);

    Testing::setAccessibleName(mainWidget, qsl("AS ShortcutsPage mainWidget"));
    scrollArea->setWidget(mainWidget);

    auto layout = Utils::emptyHLayout(_parent);
    Testing::setAccessibleName(scrollArea, qsl("AS ShortcutsPage scrollArea"));
    layout->addWidget(scrollArea);
    auto keysHeader = QT_TRANSLATE_NOOP("settings", "Keys");
    {
        auto headerWidget = new QWidget(scrollArea);
        auto headerLayout = Utils::emptyVLayout(headerWidget);
        headerLayout->setSpacing(0);
        Utils::grabTouchWidget(headerWidget);
        headerLayout->setAlignment(Qt::AlignTop);
        headerLayout->setContentsMargins(Utils::scale_value(20), 0, Utils::scale_value(20), 0);

        if constexpr (platform::is_apple())
            GeneralCreator::addHeader(headerWidget, headerLayout, keysHeader % u' ' % KeySymbols::Mac::command % u" + W");
        else
            GeneralCreator::addHeader(headerWidget, headerLayout, keysHeader % u' ' % u"Ctrl + W");

        mainLayout->addWidget(headerWidget);
        mainLayout->addSpacing(Utils::scale_value(8));

        auto currentAct = ShortcutsCloseAction::Default;
        auto actCode = get_gui_settings()->get_value<int>(settings_shortcuts_close_action, static_cast<int>(ShortcutsCloseAction::Default));
        if (actCode > static_cast<int>(ShortcutsCloseAction::min) && actCode < static_cast<int>(ShortcutsCloseAction::max))
            currentAct = static_cast<ShortcutsCloseAction>(actCode);

        auto closeWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(closeWidget);
        auto closeLayout = new QVBoxLayout(closeWidget);
        closeLayout->setAlignment(Qt::AlignTop);
        closeLayout->setContentsMargins(0, 0, 0, 0);
        closeLayout->setSpacing(Utils::scale_value(12));

        auto list = new SimpleListWidget(Qt::Vertical);
        const auto& actsList = Utils::getShortcutsCloseActionsList();
        auto selectedIndex = 0, i = 0;
        for (const auto&[name, code] : actsList)
        {
            auto row = new RadioTextRow(name);
            switch (code)
            {
            case ShortcutsCloseAction::CloseChat: Testing::setAccessibleName(row, qsl("AS ShortcutsPage closeChat")); break;
            case ShortcutsCloseAction::RollUpWindow: Testing::setAccessibleName(row, qsl("AS ShortcutsPage rollUpWindow")); break;
            case ShortcutsCloseAction::RollUpWindowAndChat: Testing::setAccessibleName(row, qsl("AS ShortcutsPage rollUpWindowAndChat")); break;
            default: Testing::setAccessibleName(row, qsl("AS ShortcutsPage unknown"));
            }
            list->addItem(row);
            if (currentAct == code)
                selectedIndex = i;
            ++i;
        }
        list->setCurrentIndex(selectedIndex);

        QObject::connect(list, &SimpleListWidget::clicked, list, [&actsList, list](int idx) {
            if (idx < 0 && idx >= static_cast<int>(actsList.size()))
                return;
            get_gui_settings()->set_value<int>(settings_shortcuts_close_action, static_cast<int>(actsList[idx].second));
            list->setCurrentIndex(idx);
            Utils::InterConnector::instance().getMainWindow()->updateMainMenu();
        }, Qt::UniqueConnection);

        closeLayout->addWidget(list);
        mainLayout->addWidget(closeWidget);
        mainLayout->addSpacing(Utils::scale_value(40));
    }
    {
        auto headerWidget = new QWidget(scrollArea);
        auto headerLayout = Utils::emptyVLayout(headerWidget);
        headerLayout->setSpacing(0);
        Utils::grabTouchWidget(headerWidget);
        headerLayout->setAlignment(Qt::AlignTop);
        headerLayout->setContentsMargins(Utils::scale_value(20), 0, Utils::scale_value(20), 0);

        if constexpr (platform::is_apple())
            GeneralCreator::addHeader(headerWidget, headerLayout, keysHeader % u' ' % KeySymbols::Mac::command % u" + F");
        else
            GeneralCreator::addHeader(headerWidget, headerLayout, keysHeader % u' ' % u"Ctrl + F");

        mainLayout->addWidget(headerWidget);
        mainLayout->addSpacing(Utils::scale_value(8));

        auto currentAct = ShortcutsSearchAction::Default;
        auto actCode = get_gui_settings()->get_value<int>(settings_shortcuts_search_action, static_cast<int>(ShortcutsSearchAction::Default));
        if (actCode > static_cast<int>(ShortcutsSearchAction::min) && actCode < static_cast<int>(ShortcutsSearchAction::max))
            currentAct = static_cast<ShortcutsSearchAction>(actCode);

        auto searchWidget = new QWidget(scrollArea);
        Utils::grabTouchWidget(searchWidget);
        auto searchLayout = new QVBoxLayout(searchWidget);
        searchLayout->setAlignment(Qt::AlignTop);
        searchLayout->setContentsMargins(0, 0, 0, 0);
        searchLayout->setSpacing(Utils::scale_value(12));

        QString reverseSuffix;
        if constexpr (platform::is_apple())
            reverseSuffix = u'(' % KeySymbols::Mac::shift % KeySymbols::Mac::command % u"F)";
        else
            reverseSuffix = qsl("(Ctrl + Shift + F)");

        const auto& actsList = Utils::getShortcutsSearchActionsList();
        auto list = new SimpleListWidget(Qt::Vertical);
        auto selectedIndex = 0, i = 0;
        for (const auto&[name, code] : actsList)
        {
            auto row = new RadioTextRow(name);
            switch (code)
            {
            case ShortcutsSearchAction::GlobalSearch : Testing::setAccessibleName(row, qsl("AS ShortcutsPage globalSearch")); break;
            case ShortcutsSearchAction::SearchInChat : Testing::setAccessibleName(row, qsl("AS ShortcutsPage searchInChat")); break;
            default: Testing::setAccessibleName(row, qsl("AS ShortcutsPage unknown"));
            }
            if (currentAct == code)
                selectedIndex = i;
            else
                row->setComment(reverseSuffix);

            list->addItem(row);
            ++i;
        }
        list->setCurrentIndex(selectedIndex);

        QObject::connect(list, &SimpleListWidget::clicked, list, [&actsList, list, reverseSuffix](int idx) {
            if (idx < 0 && idx >= static_cast<int>(actsList.size()))
                return;

            get_gui_settings()->set_value<int>(settings_shortcuts_search_action, static_cast<int>(actsList[idx].second));

            auto prev_row = dynamic_cast<RadioTextRow*>(list->itemAt(list->getCurrentIndex()));
            prev_row->setComment(reverseSuffix);

            list->setCurrentIndex(idx);
            auto current_row = dynamic_cast<RadioTextRow*>(list->itemAt(idx));
            current_row->resetComment();
        }, Qt::UniqueConnection);

        searchLayout->addWidget(list);
        mainLayout->addWidget(searchWidget);
        mainLayout->addSpacing(Utils::scale_value(40));
    }
    {
        const auto& keysIndex = Utils::getSendKeysIndex();
        auto currentKey = get_gui_settings()->get_value<int>(settings_key1_to_send_message, KeyToSendMessage::Enter);
        auto selectedIndex = 0, i = 0;
        std::vector<QString> values;
        values.reserve(keysIndex.size());
        for (const auto&[name, key] : keysIndex)
        {
            values.push_back(name);
            if (currentKey == key)
                selectedIndex = i;
            ++i;
        }

        auto keyWidget = new QWidget(scrollArea);
        auto keyLayout = Utils::emptyVLayout(keyWidget);
        keyLayout->setSpacing(0);
        Utils::grabTouchWidget(keyWidget);
        keyLayout->setAlignment(Qt::AlignTop);
        keyLayout->setContentsMargins(Utils::scale_value(20), 0, Utils::scale_value(16), 0);

        GeneralCreator::addDropper(
            keyWidget,
            keyLayout,
            QT_TRANSLATE_NOOP("settings", "Send message by"),
            true,
            values,
            selectedIndex,
            -1,
            [&keysIndex](const QString& v, int ix, TextEmojiWidget*)
            {
                get_gui_settings()->set_value<int>(settings_key1_to_send_message, keysIndex[ix].second);
            });

        mainLayout->addWidget(keyWidget);
        mainLayout->addSpacing(Utils::scale_value(40));
    }
    {
        auto headerWidget = new QWidget(scrollArea);
        auto headerLayout = Utils::emptyVLayout(headerWidget);
        Utils::grabTouchWidget(headerWidget);
        headerLayout->setSpacing(0);
        headerLayout->setAlignment(Qt::AlignTop);
        headerLayout->setContentsMargins(Utils::scale_value(20), 0, Utils::scale_value(16), 0);

        GeneralCreator::addHeader(headerWidget, headerLayout, QT_TRANSLATE_NOOP("settings", "Keyboard shortcuts"));
        mainLayout->addWidget(headerWidget);
        mainLayout->addSpacing(Utils::scale_value(12));

        const auto shortcutBlock = [mainWidget, mainLayout, scrollArea](const auto& _header)
        {
            auto infoWidget = new QWidget(mainWidget);
            auto infoLayout = Utils::emptyVLayout(infoWidget);
            Utils::grabTouchWidget(infoWidget);
            infoLayout->setAlignment(Qt::AlignTop);
            infoLayout->setContentsMargins(Utils::scale_value(20), Utils::scale_value(14), Utils::scale_value(16), Utils::scale_value(52));
            infoLayout->setSpacing(Utils::scale_value(16));

            GeneralCreator::addHeader(scrollArea, mainLayout, _header, 20);
            mainLayout->addSpacing(Utils::scale_value(12));
            mainLayout->addWidget(infoWidget);

            return std::make_pair(infoWidget, infoLayout);
        };

        {
            auto [mainWindowW, mainWindowL] = shortcutBlock(QT_TRANSLATE_NOOP("settings", "Main window"));

            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('Q');
                else
                    keys = qsl("Ctrl + Q");

                GeneralCreator::addHotkeyInfo(
                    mainWindowW, mainWindowL,
                    QT_TRANSLATE_NOOP("shortcuts", "Quit an application"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('M');
                else
                    keys = qsl("Win + ") % KeySymbols::arrow_down;

                GeneralCreator::addHotkeyInfo(
                    mainWindowW, mainWindowL,
                    QT_TRANSLATE_NOOP("shortcuts", "Minimize the window"),
                    keys);
            }
            {
                if constexpr (platform::is_apple())
                {
                    QString keys = KeySymbols::Mac::command % ql1c('0');

                    GeneralCreator::addHotkeyInfo(
                        mainWindowW, mainWindowL,
                        QT_TRANSLATE_NOOP("shortcuts", "Show main window"),
                        keys);
                }
            }
            {
                if constexpr (platform::is_apple())
                {
                    QString keys = KeySymbols::Mac::command % ql1c('H');

                    GeneralCreator::addHotkeyInfo(
                        mainWindowW, mainWindowL,
                        QT_TRANSLATE_NOOP("shortcuts", "Close main window"),
                        keys);
                }
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::control % KeySymbols::Mac::command % ql1c('F');
                else
                    keys = qsl("Win + ") % KeySymbols::arrow_up;

                GeneralCreator::addHotkeyInfo(
                    mainWindowW, mainWindowL,
                    QT_TRANSLATE_NOOP("shortcuts", "Open window in fullscreen"),
                    keys);
            }
            {
                if constexpr (platform::is_apple())
                {
                    if (config::get().is_on(config::features::add_contact))
                    {
                        GeneralCreator::addHotkeyInfo(
                            mainWindowW, mainWindowL,
                            QT_TRANSLATE_NOOP("shortcuts", "Add contact"),
                            KeySymbols::Mac::command % ql1c('N'));
                    }
                }
            }
            {
                if constexpr (platform::is_apple())
                {
                    GeneralCreator::addHotkeyInfo(
                        mainWindowW, mainWindowL,
                        QT_TRANSLATE_NOOP("shortcuts", "Open settings"),
                        KeySymbols::Mac::command % ql1c(','));
                }
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('S');
                else
                    keys = qsl("Ctrl + S");

                _parent->statuses_ = GeneralCreator::addHotkeyInfo(
                        mainWindowW, mainWindowL,
                        QT_TRANSLATE_NOOP("shortcuts", "Set status"),
                        keys);
            }
        }

        {
            auto [galleryW, galleryL] = shortcutBlock(QT_TRANSLATE_NOOP("settings", "Gallery"));
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('+');
                else
                    keys = qsl("Ctrl + +");

                GeneralCreator::addHotkeyInfo(
                    galleryW, galleryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Zoom in image"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('-');
                else
                    keys = qsl("Ctrl + -");

                GeneralCreator::addHotkeyInfo(
                    galleryW, galleryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Zoom out image"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % ql1c('0');
                else
                    keys = qsl("Ctrl + 0");

                GeneralCreator::addHotkeyInfo(
                    galleryW, galleryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Set default size for image"),
                    keys);
            }
        }
        {
            auto[recentChatsW, recentChatsL] = shortcutBlock(QT_TRANSLATE_NOOP("settings", "Recent chats"));
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::control % KeySymbols::Mac::shift % KeySymbols::Mac::tab;
                else
                    keys = qsl("Ctrl + Shift + Tab");

                GeneralCreator::addHotkeyInfo(
                    recentChatsW, recentChatsL,
                    QT_TRANSLATE_NOOP("shortcuts", "Go to the previous dialog"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::control % KeySymbols::Mac::tab;
                else
                    keys = qsl("Ctrl + Tab");

                GeneralCreator::addHotkeyInfo(
                    recentChatsW, recentChatsL,
                    QT_TRANSLATE_NOOP("shortcuts", "Go to the next dialog"),
                    keys);
            }
            {
                if constexpr (platform::is_apple())
                {
                    QString keys = KeySymbols::Mac::command % ql1c(']');

                    GeneralCreator::addHotkeyInfo(
                        recentChatsW, recentChatsL,
                        QT_TRANSLATE_NOOP("shortcuts", "Go to the first unread dialog"),
                        keys);
                }
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::control % ql1c('R');
                else
                    keys = qsl("Ctrl + R");

                GeneralCreator::addHotkeyInfo(
                    recentChatsW, recentChatsL,
                    QT_TRANSLATE_NOOP("shortcuts", "Mark all dialogs as read"),
                    keys);
            }
        }
        {
            auto[chatHistoryW, chatHistoryL] = shortcutBlock(QT_TRANSLATE_NOOP("settings", "History in chat"));
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = u"fn" % KeySymbols::arrow_up % u'/' % KeySymbols::arrow_down;
                else
                    keys = qsl("PgUp/PgDown");

                GeneralCreator::addHotkeyInfo(
                    chatHistoryW, chatHistoryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Scroll history in chat"),
                    keys);
            }
            {
                GeneralCreator::addHotkeyInfo(
                    chatHistoryW, chatHistoryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Edit the last message"),
                    KeySymbols::arrow_up);

            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::option % KeySymbols::arrow_up;
                else
                    keys = u"Shift + Alt + " % KeySymbols::arrow_up;

                GeneralCreator::addHotkeyInfo(
                    chatHistoryW, chatHistoryL,
                    QT_TRANSLATE_NOOP("shortcuts", "Enter multiselect mode"),
                    keys);
            }
        }
        {
            auto[chatInputW, chatInputL] = shortcutBlock(QT_TRANSLATE_NOOP("settings", "Input in chat"));
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::command % ql1c('V');
                else
                    keys = qsl("Ctrl + Shift + V");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Paste text without formatting"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::command % ql1c('Z');
                else if constexpr (platform::is_windows())
                    keys = qsl("Ctrl + Y");
                else
                    keys = qsl("Ctrl + Shift + Z");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Repeat last action"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % KeySymbols::arrow_right;
                else
                    keys = qsl("End");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the end of current line"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % KeySymbols::arrow_left;
                else
                    keys = qsl("Home");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the beginning of current line"),
                    keys);

            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = u"fn" % KeySymbols::arrow_right;
                else
                    keys = qsl("Ctrl + End");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the end of the message"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = u"fn" % KeySymbols::arrow_left;
                else
                    keys = qsl("Ctrl + Home");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the beginning of the message"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::command % KeySymbols::arrow_up;
                else
                    keys = u"Ctrl + Shift + " % KeySymbols::arrow_up;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select all to the left of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::command % KeySymbols::arrow_down;
                else
                    keys = u"Ctrl + Shift + " % KeySymbols::arrow_down;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select all to the right of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::option % KeySymbols::Mac::shift % KeySymbols::arrow_right;
                else
                    keys = u"Ctrl + Shift + " % KeySymbols::arrow_right;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select all to the right of the cursor (only current word)"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::option % KeySymbols::Mac::shift % KeySymbols::arrow_left;
                else
                    keys = u"Ctrl + Shift + " % KeySymbols::arrow_left;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select all to the left of the cursor (only current word)"),
                    keys);
            }
            {
                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Go to the previous line"),
                    KeySymbols::arrow_up);

            }
            {
                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Go to the next line"),
                    KeySymbols::arrow_down);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::option % KeySymbols::Mac::backspace;
                else
                    keys = qsl("Ctrl + Backspace");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Delete the current word to the left of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = u"fn" % KeySymbols::Mac::option % KeySymbols::Mac::backspace;
                else
                    keys = qsl("Ctrl + Delete");

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Delete the current word to the right of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::arrow_right;
                else
                    keys = u"Shift + " % KeySymbols::arrow_right;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select one symbol to the right of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::arrow_left;
                else
                    keys = u"Shift + " % KeySymbols::arrow_left;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select one symbol to the left of the cursor"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::arrow_up % u'/' % KeySymbols::arrow_down;
                else
                    keys = u"Shift + " % KeySymbols::arrow_up % u'/' % KeySymbols::arrow_down;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Select the line"),
                    keys);
            }
            {
                if constexpr (platform::is_apple())
                {
                    GeneralCreator::addHotkeyInfo(
                        chatInputW, chatInputL,
                        QT_TRANSLATE_NOOP("shortcuts", "Open native Emoji panel"),
                        KeySymbols::Mac::control % KeySymbols::Mac::command % QT_TRANSLATE_NOOP("shortcuts", "Space"));
                }
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % KeySymbols::arrow_up;
                else
                    keys = u"Ctrl + " % KeySymbols::arrow_up;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the beginning of the paragraph"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::command % KeySymbols::arrow_down;
                else
                    keys = u"Ctrl + " % KeySymbols::arrow_down;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the end of the paragraph"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::option % KeySymbols::arrow_left;
                else
                    keys = u"Ctrl + " % KeySymbols::arrow_left;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the beginning of the word"),
                    keys);
            }
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::option % KeySymbols::arrow_right;
                else
                    keys = u"Ctrl + " % KeySymbols::arrow_right;

                GeneralCreator::addHotkeyInfo(
                    chatInputW, chatInputL,
                    QT_TRANSLATE_NOOP("shortcuts", "Move to the end of the word"),
                    keys);
            }
        }
        {
            auto res = shortcutBlock(QT_TRANSLATE_NOOP("settings", "Call window"));
            auto addShortcut = [videoCallWindowW = res.first, videoCallWindowL = res.second](const QString _shortcutLetter, const QString& _device)
            {
                QString keys;
                if constexpr (platform::is_apple())
                    keys = KeySymbols::Mac::shift % KeySymbols::Mac::command % _shortcutLetter;
                else
                    keys = qsl("Ctrl + Shift + %1").arg(_shortcutLetter);

                GeneralCreator::addHotkeyInfo(
                    videoCallWindowW, videoCallWindowL,
                    QT_TRANSLATE_NOOP("shortcuts", "Switch %1 state").arg(_device),
                    keys);
            };

            addShortcut(qsl("A"), QT_TRANSLATE_NOOP("shortcuts", "microphone"));
            addShortcut(qsl("V"), QT_TRANSLATE_NOOP("shortcuts", "video"));
            addShortcut(qsl("S"), QT_TRANSLATE_NOOP("shortcuts", "presentation"));
            addShortcut(qsl("D"), QT_TRANSLATE_NOOP("shortcuts", "sound"));
        }
    }
}
/*
Shortcuts keys:

Shift+Cmd+Z/Ctrl+Y/Ctrl+Shift+Z - Repeat last action
Cmd+ArrowRight/End or Fn+ArrowRight - Move to the end of current line
Cmd+ArrowLeft/Home or Fn+ArrowLeft - Move to the beginning of current line
Fn+ArrowRight/Ctrl+End or Ctrl+Fn+ArrowRight - Move to the end of the message
Fn+ArrowLeft/Ctrl+Home or Ctrl+Fn+ArrowLeft - Move to the beginning of the message
Ctrl+Shift+ArrowUp or Cmd+Shift+ArrowUp/Ctrl+Shift+ArrowUp - Select all to the right of the cursor
Ctrl+Shift+ArrowDown or Cmd+Shift+ArrowDown/Ctrl+Shift+ArrowDown - Select all to the left of the cursor
Option+Shift+ArrowRight/Ctrl+Shift+ArrowRight - Select all to the right of the cursor (only current word)
Option+Shift+ArrowLeft/Ctrl+Shift+ArrowLeft - Select all to the left of the cursor  (only current word)
ArrowUp - Go to the previous line
ArrowDown - Go to the next line
ArrowUp - Edit the last message
Option+Backspace/Ctrl+Backspace - Delete the current word to the left of the cursor
Fn+Option+Backspace/Ctrl+Delete - Delete the current word to the right of the cursor
Shift+ArrowRight - Select one symbol to the right of the cursor
Shift+ArrowLeft - Select one symbol to the left of the cursor
Shift+ArrowUp or Shift+ArrowDown - Select the line
Ctrl+Cmd+Space/? - Open native Emoji panel
Cmd+ArrowUp/Ctrl+ArrowUp - Move to the beginning of the paragraph
Cmd+ArrowDown/Ctrl+ArrowDown - Move to the end of the paragraph
Option+ArrowLeft/Ctrl+ArrowLeft - Move to the beginning of the word
Option+ArrowRight/Ctrl+ArrowRight - Move to the end of the word
Cmd+Shift+A/Ctrl+Shift+A - switch microphone state in call window
Cmd+Shift+V/Ctrl+Shift+V - switch video state in call window
Cmd+Shift+D/Ctrl+Shift+D - switch sound state in call window
Cmd+Shift+S/Ctrl+Shift+S - switch screen sharing state in call window
*/
