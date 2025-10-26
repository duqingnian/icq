#pragma once

namespace Styling
{
    class StylingParams : public QObject
    {
        Q_OBJECT

    public:
        StylingParams(QObject* _parent)
            : QObject(_parent)
        {
        }

        enum class StyleVariable
        {
            BASE,
            BASE_LIGHT,
            BASE_PRIMARY,
            BASE_PRIMARY_HOVER,
            BASE_PRIMARY_ACTIVE,
            BASE_SECONDARY,
            BASE_SECONDARY_HOVER,
            BASE_SECONDARY_ACTIVE,
            BASE_TERTIARY,
            BASE_BRIGHT,
            BASE_BRIGHT_ACTIVE,
            BASE_BRIGHT_HOVER,
            BASE_ULTRABRIGHT,
            BASE_ULTRABRIGHT_HOVER,
            BASE_BRIGHT_INVERSE,
            BASE_BRIGHT_INVERSE_HOVER,
            TEXT_SOLID,
            TEXT_SOLID_PERMANENT,
            TEXT_SOLID_PERMANENT_HOVER,
            TEXT_SOLID_PERMANENT_ACTIVE,
            TEXT_PRIMARY,
            TEXT_PRIMARY_HOVER,
            TEXT_PRIMARY_ACTIVE,
            BASE_GLOBALWHITE,
            BASE_GLOBALWHITE_PERMANENT,
            BASE_GLOBALWHITE_PERMANENT_HOVER,
            BASE_GLOBALWHITE_SECONDARY,
            BASE_GLOBALBLACK_PERMANENT,
            PRIMARY,
            PRIMARY_HOVER,
            PRIMARY_ACTIVE,
            PRIMARY_PASTEL,
            PRIMARY_SELECTED,
            PRIMARY_INVERSE,
            PRIMARY_BRIGHT,
            PRIMARY_BRIGHT_HOVER,
            PRIMARY_BRIGHT_ACTIVE,
            PRIMARY_LIGHT,
            SECONDARY_INFO,
            SECONDARY_RAINBOW_WARNING,
            SECONDARY_RAINBOW_MINT,
            SECONDARY_RAINBOW_COLD,
            SECONDARY_RAINBOW_WARM,
            SECONDARY_RAINBOW_ORANGE,
            SECONDARY_RAINBOW_AQUA,
            SECONDARY_RAINBOW_PURPLE,
            SECONDARY_RAINBOW_PINK,
            SECONDARY_RAINBOW_BLUE,
            SECONDARY_RAINBOW_GREEN,
            SECONDARY_RAINBOW_MAIL,
            SECONDARY_RAINBOW_YELLOW,
            SECONDARY_RAINBOW_EMERALD,
            SECONDARY_RAINBOW_EMERALD_HOVER,
            SECONDARY_RAINBOW_EMERALD_ACTIVE,
            SECONDARY_ATTENTION,
            SECONDARY_ATTENTION_HOVER,
            SECONDARY_ATTENTION_ACTIVE,
            CHAT_ENVIRONMENT,
            CHAT_THREAD_ENVIRONMENT,
            CHAT_PRIMARY,
            CHAT_PRIMARY_HOVER,
            CHAT_PRIMARY_ACTIVE,
            CHAT_PRIMARY_MEDIA,
            CHAT_SECONDARY,
            CHAT_SECONDARY_MEDIA,
            CHAT_TERTIARY,
            GHOST_PRIMARY,
            GHOST_PRIMARY_INVERSE,
            GHOST_PRIMARY_INVERSE_HOVER,
            GHOST_PRIMARY_INVERSE_ACTIVE,
            GHOST_SECONDARY,
            GHOST_SECONDARY_INVERSE,
            GHOST_SECONDARY_INVERSE_HOVER,
            GHOST_LIGHT,
            GHOST_LIGHT_INVERSE,
            GHOST_TERTIARY,
            GHOST_ULTRALIGHT,
            GHOST_ULTRALIGHT_INVERSE,
            GHOST_ACCENT,
            GHOST_ACCENT_HOVER,
            GHOST_ACCENT_ACTIVE,
            GHOST_OVERLAY,
            GHOST_BORDER_INVERSE,
            GHOST_QUATERNARY,
            GHOST_PERCEPTIBLE,
            LUCENT_TERTIARY,
            LUCENT_TERTIARY_HOVER,
            LUCENT_TERTIARY_ACTIVE,
            APP_PRIMARY,
            TYPING_TEXT,
            TYPING_ANIMATION,
            CHATEVENT_TEXT,
            CHATEVENT_BACKGROUND,
            NEWPLATE_BACKGROUND,
            NEWPLATE_TEXT,
            TASK_CHEAPS_CREATE,
            TASK_CHEAPS_CREATE_HOVER,
            TASK_CHEAPS_CREATE_ACTIVE,
            TASK_CHEAPS_IN_PROGRESS,
            TASK_CHEAPS_IN_PROGRESS_HOVER,
            TASK_CHEAPS_IN_PROGRESS_ACTIVE,
            TASK_CHEAPS_DENIED,
            TASK_CHEAPS_DENIED_HOVER,
            TASK_CHEAPS_DENIED_ACTIVE,
            TASK_CHEAPS_READY,
            TASK_CHEAPS_READY_HOVER,
            TASK_CHEAPS_READY_ACTIVE,
            TASK_CHEAPS_CLOSED,
            TASK_CHEAPS_CLOSED_HOVER,
            TASK_CHEAPS_CLOSED_ACTIVE,
            SECONDARY_SELECTED,

            INVALID
        };
        Q_ENUM(StyleVariable);
    };

    using StyleVariable = StylingParams::StyleVariable;
    using StyleVariableStrings = std::vector<std::pair<StyleVariable, QStringView>>;
    const StyleVariableStrings& getVariableStrings();

    QStringView variableToString(StyleVariable _var);
    StyleVariable stringToVariable(QStringView _name);
} // namespace Styling
