#include "MessageItemMemMonitor.h"
#include "../utils/QObjectWatcher.h"
#include "../main_window/history_control/complex_message/ComplexMessageItem.h"

#include "../main_window/history_control/complex_message/FileSharingBlock.h"
#include "../main_window/history_control/complex_message/PttBlock.h"
#include "../main_window/history_control/complex_message/TextBlock.h"
#include "../main_window/history_control/complex_message/DebugTextBlock.h"
#include "../main_window/history_control/complex_message/QuoteBlock.h"
#include "../main_window/history_control/complex_message/StickerBlock.h"

#include "../utils/memory_utils.h"


MessageItemMemMonitor& MessageItemMemMonitor::instance()
{
    static MessageItemMemMonitor memMonitor(nullptr);


    return memMonitor;
}

MessageItemMemMonitor::CategoriesArray MessageItemMemMonitor::getMessageItemsFootprint()
{
    Category countBlocks(0, 0),
             textBlocks(0, 0),
             imgPreviewBlocks(0, 0),
             fileSharingBlocks(0, 0),
             snippetBlocks(0, 0),
             stickersBlocks(0, 0),
             quotesBlocks(0, 0);

    for (auto messageItemObj: messageItemsWatcher()->allObjects())
    {
        auto messageItem = qobject_cast<Ui::ComplexMessage::ComplexMessageItem*>(messageItemObj);
        im_assert(messageItem);
        if (!messageItem)
            continue;

        auto blocks = messageItem->getBlocks();
        countBlocks.first += blocks.size();

        for (auto &block: blocks)
        {
            switch (block->getContentType())
            {
            case Ui::ComplexMessage::IItemBlock::ContentType::Text:
            {
                auto textBlock = tryGetTextBlock(block);
                if (textBlock)
                {
                    textBlocks.first++;
                    textBlocks.second += Utils::getMemoryFootprint(textBlock);
                }
            }
                break;
            case Ui::ComplexMessage::IItemBlock::ContentType::Link:
            {
                auto snippetBlock = tryGetSnippetBlock(block);
                if (snippetBlock)
                {
                    snippetBlocks.first++;
                    snippetBlocks.second += Utils::getMemoryFootprint(snippetBlock);
                    break;
                }

                im_assert(!"what else could it be");
            }
            case Ui::ComplexMessage::IItemBlock::ContentType::Sticker:
            {
                auto stickerBlock = tryGetStickerBlock(block);
                if (stickerBlock)
                {
                    stickersBlocks.first++;
                    stickersBlocks.second += Utils::getMemoryFootprint(stickerBlock);
                }
            }
                break;
            case Ui::ComplexMessage::IItemBlock::ContentType::FileSharing:
            {
                auto fileSharingBlock = tryGetFileSharingBlock(block);
                if (fileSharingBlock)
                {
                    fileSharingBlocks.first++;
                    fileSharingBlocks.second += Utils::getMemoryFootprint(fileSharingBlock);
                }
            }
                break;
            case Ui::ComplexMessage::IItemBlock::ContentType::Quote:
            {
                auto quoteBlock = tryGetQuoteBlock(block);
                if (quoteBlock)
                {
                    quotesBlocks.first++;
                    quotesBlocks.second += Utils::getMemoryFootprint(quoteBlock);
                }
            }
                break;

            default:
                break;
            }
        }
    }

    countBlocks.second += (quotesBlocks.second + fileSharingBlocks.second + stickersBlocks.second
                           + snippetBlocks.second + imgPreviewBlocks.second + textBlocks.second);


    return { countBlocks, textBlocks, imgPreviewBlocks, snippetBlocks, stickersBlocks, fileSharingBlocks, quotesBlocks };
}



MessageItemMemMonitor::MessageItemMemMonitor(QObject *parent)
    : QObject(parent)
{
}

Utils::QObjectWatcher *MessageItemMemMonitor::messageItemsWatcher()
{
    if (!messageItemsWatcher_)
        messageItemsWatcher_ = new Utils::QObjectWatcher(this);

    return messageItemsWatcher_;
}

Ui::ComplexMessage::TextBlock* MessageItemMemMonitor::tryGetTextBlock(Ui::ComplexMessage::IItemBlock *_itemBlock)
{
    return dynamic_cast<Ui::ComplexMessage::TextBlock *>(_itemBlock);
}

Ui::ComplexMessage::SnippetBlock *MessageItemMemMonitor::tryGetSnippetBlock(Ui::ComplexMessage::IItemBlock *_itemBlock)
{
    return dynamic_cast<Ui::ComplexMessage::SnippetBlock *>(_itemBlock);
}

Ui::ComplexMessage::StickerBlock *MessageItemMemMonitor::tryGetStickerBlock(Ui::ComplexMessage::IItemBlock *_stickerBlock)
{
    return dynamic_cast<Ui::ComplexMessage::StickerBlock *>(_stickerBlock);
}

Ui::ComplexMessage::FileSharingBlock *MessageItemMemMonitor::tryGetFileSharingBlock(Ui::ComplexMessage::IItemBlock *_fileSharingBlock)
{
    return dynamic_cast<Ui::ComplexMessage::FileSharingBlock *>(_fileSharingBlock);
}

Ui::ComplexMessage::QuoteBlock *MessageItemMemMonitor::tryGetQuoteBlock(Ui::ComplexMessage::IItemBlock *_quoteBlock)
{
    return dynamic_cast<Ui::ComplexMessage::QuoteBlock *>(_quoteBlock);
}

bool MessageItemMemMonitor::watchComplexMsgItem(Ui::ComplexMessage::ComplexMessageItem* _msgItem)
{
    return messageItemsWatcher()->addObject(qobject_cast<QObject *>(_msgItem));
}
