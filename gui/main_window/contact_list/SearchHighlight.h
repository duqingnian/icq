#pragma once

#include "controls/TextUnit.h"

namespace Ui
{
    struct HighlightFindResult
    {
        int indexStart_;
        int length_;
        HighlightFindResult(const int _ndx, const int _len) : indexStart_(_ndx), length_(_len) {}
    };

    enum class HighlightPosition
    {
        anywhere,
        wordStart
    };

    Styling::ThemeColorKey getHighlightColor();
    Styling::ThemeColorKey getTextHighlightedColor();

    using highlightsV = std::vector<QString>;

    HighlightFindResult findNextHighlight(
        QStringView _text,
        const highlightsV& _highlights,
        const int _posStart = 0,
        const HighlightPosition _hlPos = HighlightPosition::anywhere);

    TextRendering::TextUnitPtr createHightlightedText(
        const QString& _text,
        const QFont& _font,
        const Styling::ThemeColorKey& _baseColor,
        const Styling::ThemeColorKey& _hlColor,
        const int _maxLines,
        const highlightsV& _highlights = {},
        const Data::MentionMap& _mentions = {},
        const HighlightPosition _hlPos = HighlightPosition::anywhere);
}
