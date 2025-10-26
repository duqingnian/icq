#include "stdafx.h"

#include "profiling/auto_stop_watch.h"
#include "utils.h"

#include "Text.h"

namespace Utils
{
    TextHeightMetrics::TextHeightMetrics(const int32_t heightLines, const int32_t lineHeight)
        : HeightLines_(heightLines)
        , LineHeight_(lineHeight)
    {
        im_assert(HeightLines_ > 0);
        im_assert(LineHeight_ > 0);
    }

    int32_t TextHeightMetrics::getHeightPx() const
    {
        im_assert(HeightLines_ > 0);
        im_assert(LineHeight_ > 0);

        return (HeightLines_ * LineHeight_);
    }

    int32_t TextHeightMetrics::getHeightLines() const
    {
        return HeightLines_;
    }

    int32_t TextHeightMetrics::getLineHeight() const
    {
        return LineHeight_;
    }

    bool TextHeightMetrics::isEmpty() const
    {
        im_assert(HeightLines_ > 0);
        return (HeightLines_ <= 0);
    }

    TextHeightMetrics TextHeightMetrics::linesNumberChanged(const int32_t delta)
    {
        im_assert(delta != 0);

        const auto newHeightLines = (getHeightLines() + delta);
        im_assert(newHeightLines > 0);

        return TextHeightMetrics(newHeightLines, getLineHeight());
    }

    int applyMultilineTextFix(const int32_t textHeight, const int32_t contentHeight)
    {
        im_assert(textHeight > 0);
        im_assert(contentHeight > 0);
        if constexpr (platform::is_windows())
        {
            const auto isMultiline = (textHeight >= Utils::scale_value(38));
            const auto fix = (
                isMultiline ?
                    Utils::scale_value(9) :
                    Utils::scale_value(-4)
            );

            return (contentHeight + fix);
        }
        else if constexpr (platform::is_linux())
        {
            const auto isMultiline = (textHeight >= Utils::scale_value(34));
            const auto fix = (
                isMultiline ?
                    Utils::scale_value(7) :
                    Utils::scale_value(-7)
            );

            return (contentHeight + fix);
        }
        else if constexpr (platform::is_apple())
        {
            const auto isMultiline = (textHeight >= Utils::scale_value(36));
            const auto fix = (
                isMultiline ?
                    Utils::scale_value(10) :
                    Utils::scale_value(-5)
            );

            return (contentHeight + fix);
        }

        return contentHeight;
    }

    TextHeightMetrics evaluateTextHeightMetrics(const QString &text, const int32_t textWidth, const QFontMetrics &fontMetrics)
    {
        im_assert(textWidth > 0);

        // preliminary fix of division by zero
        // todo: replace the default value when required
        const auto fontHeight = std::max(fontMetrics.height(), 1);

        if (text.isEmpty() || (textWidth <= 0))
        {
            return TextHeightMetrics(1, fontHeight);
        }

        const QRect textLtr(0, 0, textWidth, QWIDGETSIZE_MAX / 2);
        const auto textRect = fontMetrics.boundingRect(textLtr, Qt::AlignLeft | Qt::TextWordWrap, text);
        const auto textHeight = textRect.height();
        const auto linesNumber = (1 + ((textHeight - 1) / fontHeight));
        im_assert(linesNumber >= 1);

        return TextHeightMetrics(linesNumber, fontHeight);
    }

    int32_t evaluateActualLineHeight(const QFontMetrics &_m)
    {
        return _m.boundingRect(qsl("Mj")).height();
    }

    int32_t evaluateActualLineHeight(const QFont &_font)
    {
        return evaluateActualLineHeight(QFontMetrics(_font));
    }

    QString limitLinesNumber(const QString &text, const int32_t textWidth, const QFontMetrics &fontMetrics, const int32_t maxLinesNumber, const QString &ellipsis)
    {
        im_assert(textWidth > fontMetrics.averageCharWidth());
        im_assert(maxLinesNumber >= 1);

        const auto nothingToLimit = (
            text.isEmpty() ||
            (textWidth <= fontMetrics.averageCharWidth()));
        if (nothingToLimit)
        {
            return text;
        }

        const auto textHeightMetrics = evaluateTextHeightMetrics(text, textWidth, fontMetrics);

        const auto actualLinesNumber = textHeightMetrics.getHeightLines();

        const auto isShortEnoughAlready = (actualLinesNumber <= maxLinesNumber);
        if (isShortEnoughAlready)
        {
            return text;
        }

        const auto maxToActualRatio = ((double)maxLinesNumber / (double)actualLinesNumber);
        im_assert(maxToActualRatio < 1);

        auto limitedTextLength = (int32_t)(text.length() * maxToActualRatio) + 1;
        im_assert(limitedTextLength > 0);

        const auto ellipsisLength = ellipsis.length();
        im_assert(ellipsisLength > 0);

        limitedTextLength = std::max(limitedTextLength - ellipsisLength, 0);
        if (limitedTextLength == 0)
        {
            return text;
        }

        for (auto cursor = limitedTextLength; cursor > 0; --cursor)
        {
            const auto ch = text[cursor];
            if (ch.isSpace())
            {
                const auto textWithEllipsis = (text.left(cursor) + ellipsis);

                return textWithEllipsis;
            }
        }

        im_assert(!"something goes wrong");
        return text;
    }

    QString limitLinesNumberSlow(const QString &text, const int32_t textWidth, const QFontMetrics &fontMetrics, const int32_t maxLinesNumber, const QString &ellipsis)
    {
        im_assert(!text.isEmpty());
        im_assert(textWidth > fontMetrics.averageCharWidth());
        im_assert(maxLinesNumber > 1);

        if (textWidth <= fontMetrics.averageCharWidth())
        {
            return text;
        }

        auto splitText = text.split(ql1c(' '));

        const auto MIN_CHUNKS_NUM = 4;
        if (splitText.length() < MIN_CHUNKS_NUM)
        {
            return text;
        }

        QString limitedText;
        for (;;)
        {
            limitedText.clear();
            limitedText += splitText.join(ql1c(' ')) % ellipsis;

            if (splitText.length() < MIN_CHUNKS_NUM)
            {
                return limitedText;
            }

            const auto textHeightMetrics = evaluateTextHeightMetrics(limitedText, textWidth, fontMetrics);

            const auto actualLinesNumer = textHeightMetrics.getHeightLines();

            if (actualLinesNumer <= maxLinesNumber)
            {
                break;
            }

            splitText.removeLast();
        }

        return limitedText;
    }
}
