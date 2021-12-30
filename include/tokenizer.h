#ifndef TOKENIZER_H
#define TOKENIZER_H

namespace fex
{

    // MUST NOT BE RE-ORDERED
    // VALUES ARE BINARY SEARCHED IN CODE
    // REORDERING MAY BREAK PARSING
    enum class TokenType
    {
        SYM_HASH,
        SYM_COMMA,
        SYM_COLON,
        SYM_QUOTE,
        SYM_NEWLINE,
        SYM_PLUS,
        STRING_LIT,
        ROW_LIT,
        KEY_LIT,
        NUM_LIT,
        HEX_LIT,
        IDENTIFIER,
        OPERATION_PRESS,
        OPERATION_CLICK,
        OPERATION_HOLD,
        OPERATION_DOUBLE_CLICK,
        OPERATION_RELEASE,
        ACTION_PRESS,
        ACTION_RELEASE,
        ACTION_CLICK,
        ACTION_WAIT,
        ACTION_SWITCH_TO,
        ACTION_TOGGLE,
        ACTION_LEAVE,
        ACTION_TYPE,
        ACTION_RESET_KEYBOARD,
        ACTION_BOOTLOADER,
        ACTION_HOME,
        ACTION_NOTHING,
        ACTION_PASS_THROUGH,
        ACTION_RELOAD_KEY_MAPS,
        ACTION_MOUSE_MOVE_UP,
        ACTION_MOUSE_MOVE_DOWN,
        ACTION_MOUSE_MOVE_LEFT,
        ACTION_MOUSE_MOVE_RIGHT,
        ACTION_MOUSE_SCROLL_UP,
        ACTION_MOUSE_SCROLL_DOWN,
        ACTION_MOUSE_SCROLL_LEFT,
        ACTION_MOUSE_SCROLL_RIGHT,
        ACTION_MOUSE_CLICK_LEFT,
        ACTION_MOUSE_CLICK_RIGHT,
        ACTION_MOUSE_CLICK_CENTER,
        ACTION_MOUSE_CLICK_BACKWARDS,
        ACTION_MOUSE_CLICK_FORWARDS,
        PARAMETER_QUICKLY,
        PARAMETER_SLOWLY,
        PARAMETER_REPEATEDLY,
        PARAMETER_AT_HUMAN_SPEED,
        PARAMETER_UNTIL_RELEASED,
        PARAMETER_TIME_MS,
        PARAMETER_TIME_SEC,
        PARAMETER_TIME_MIN,
        TOP_OTHER_KEYS_FALL_THROUGH,
        TOP_BLOCK_OTHER_KEYS,
    };

    struct Token
    {
        TokenType type;
        int start;
        int length;
        int line_number;
    };

    std::string TokenStr(const std::string &source, const Token &token);
    std::string TokenRunStr(const std::string &source, const Token& start, const Token& end);

    // TODO(fex): move to utils class
    std::string errmsg(const std::string &message, int line_number);
    std::string to_lower(const std::string &s);

    std::pair<std::string, std::vector<Token>> tokenize(const std::string &source);

}

#endif
