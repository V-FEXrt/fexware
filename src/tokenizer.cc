#include <algorithm>
#include <string>
#include <vector>

#include "tokenizer.h"

namespace fex
{

    std::string errmsg(const std::string &message, int line_number)
    {
        return "Line " + std::to_string(line_number) + ": " + message;
    }

    bool isdigits(const std::string &s)
    {
        if (s.size() == 0)
        {
            return false;
        }
        for (char c : s)
        {
            if (!isdigit(c))
            {
                return false;
            }
        }
        return true;
    }

    std::string to_lower(const std::string &s)
    {
        std::string out(s);
        std::transform(s.begin(), s.end(), out.begin(), ::tolower);
        return out;
    }

    std::string TokenStr(const std::string &source, const Token &token)
    {
        return source.substr(token.start, token.length);
    }

    std::string TokenRunStr(const std::string &source, const Token &start, const Token &end)
    {
        return source.substr(start.start, end.start - start.start + end.length);
    }

    std::pair<std::string, std::vector<Token>> tokenize(const std::string &source)
    {
        std::vector<Token> tokens;

        int index = 0;
        int line_number = 1;

        while (index < source.length())
        {
            char c = source[index];

            switch (c)
            {
            case ' ':
            case '\t':
                index++;
                continue;
            case '\n':
                line_number++;
                index++;
                continue;
            case '#':
                while (index < source.length() && source[index] != '\n')
                {
                    index++;
                }

                line_number++;
                index++; // eat the '\n'
                continue;
            case ',':
                tokens.push_back({
                    .type = TokenType::SYM_COMMA,
                    .start = index,
                    .length = 1,
                    .line_number = line_number,
                });
                index++;
                continue;
            case '+':
                tokens.push_back({
                    .type = TokenType::SYM_PLUS,
                    .start = index,
                    .length = 1,
                    .line_number = line_number,
                });
                index++;
                continue;
            case ':':
                tokens.push_back({
                    .type = TokenType::SYM_COLON,
                    .start = index,
                    .length = 1,
                    .line_number = line_number,
                });
                index++;
                continue;
            case '"':
            {
                int start = index;
                index++;

                while (index < source.length() && source[index] != '"')
                    index++;

                if (source[index] != '"')
                {
                    return {errmsg("Unterminated string", line_number), {}};
                }

                index++; // Eat the "

                tokens.push_back({
                    .type = TokenType::STRING_LIT,
                    .start = start,
                    .length = index - start,
                    .line_number = line_number,
                });

                continue;
            }
            case '0':
            { // Maybe hex literal
                if (source[index + 1] != 'x')
                {
                    break; // Not a hex literal, parse as a number literal
                }
                int start = index;
                index++;
                index++; // eat the 'x'

                if (!isxdigit(source[index]))
                {
                    return {errmsg("Hex literal must have a digit", line_number), {}};
                }

                while (index < source.length() && isxdigit(source[index]))
                    index++;

                tokens.push_back({
                    .type = TokenType::HEX_LIT,
                    .start = start,
                    .length = index - start,
                    .line_number = line_number,
                });

                continue;
            }
            }

            if (isdigit(c))
            { // Number literal
                int start = index;
                while (index < source.length() && isdigit(source[index]))
                    index++;
                // if (source[index] == '.') { // allow one '.'
                //     index++;
                //     while (index < source.length() && isdigit(source[index])) index++;
                // }
                tokens.push_back({
                    .type = TokenType::NUM_LIT,
                    .start = start,
                    .length = index - start,
                    .line_number = line_number,
                });
                continue;
            }

            if (!isalnum(c))
            { // Everything else must be an identifier or keyword
                return {errmsg("Unexpected character '" + std::string{c} + "'", line_number), {}};
            }

            int start = index;
            while (index < source.length() && isalnum(source[index]))
                index++;
            int length = index - start;

            // Grab Row/Key Literals if they exist (K33, R999)
            if ((source[start] == 'R' || source[start] == 'K') && isdigits(source.substr(start + 1, length - 1)))
            {
                if (source[start] == 'R')
                {
                    tokens.push_back({
                        .type = TokenType::ROW_LIT,
                        .start = start,
                        .length = index - start,
                        .line_number = line_number,
                    });
                }
                else
                {
                    tokens.push_back({
                        .type = TokenType::KEY_LIT,
                        .start = start,
                        .length = index - start,
                        .line_number = line_number,
                    });
                }
                continue;
            }

            // Handle Keywords
            std::string identifier = to_lower(source.substr(start, length));
            TokenType type = TokenType::IDENTIFIER;

            if (identifier == "on" && source[index] == ' ')
            {
                // make a restore point
                int old_index = index;

                // move past the space
                index++;

                // look forward to see if we find the trigger
                while (index < source.length() // "-"" for double-click
                       && (isalnum(source[index]) || source[index] == '-'))
                    index++;

                length = index - start;
                std::string trigger = to_lower(source.substr(start, length));

                // Save type before starting
                TokenType old_type = type;

                if (trigger == "on press")
                    type = TokenType::OPERATION_PRESS;
                if (trigger == "on click")
                    type = TokenType::OPERATION_CLICK;
                if (trigger == "on hold")
                    type = TokenType::OPERATION_HOLD;
                if (trigger == "on double-click")
                    type = TokenType::OPERATION_DOUBLE_CLICK;
                if (trigger == "on release")
                    type = TokenType::OPERATION_RELEASE;

                // if type is still old then a trigger was not found, restore index
                if (type == old_type)
                    index = old_index;
            }

            if (identifier == "mouse" && source[index] == ' ')
            {
                // make a restore point
                int old_index = index;

                // move past the space
                index++;

                // look forward to see if we find the next keyword
                while (index < source.length() && isalpha(source[index]))
                    index++;

                int next_start = start + identifier.size() + 1;
                length = index - next_start;
                std::string mouse_kwd = to_lower(source.substr(next_start, length));

                bool is_mouse_action = false;

                std::string dir_kwd = "";
                if (mouse_kwd == "move" || mouse_kwd == "scroll" || mouse_kwd == "click")
                {
                    // move past the space
                    index++;

                    // look forward to see if we find the last keyword
                    while (index < source.length() && isalpha(source[index]))
                        index++;

                    int last_start = next_start + mouse_kwd.size() + 1;
                    length = index - last_start;
                    dir_kwd = to_lower(source.substr(last_start, length));
                }

                if (mouse_kwd == "move")
                {
                    if (dir_kwd == "up")
                    {
                        type = TokenType::ACTION_MOUSE_MOVE_UP;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "down")
                    {
                        type = TokenType::ACTION_MOUSE_MOVE_DOWN;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "left")
                    {
                        type = TokenType::ACTION_MOUSE_MOVE_LEFT;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "right")
                    {
                        type = TokenType::ACTION_MOUSE_MOVE_RIGHT;
                        is_mouse_action = true;
                    }
                }
                if (mouse_kwd == "scroll")
                {
                    if (dir_kwd == "up")
                    {
                        type = TokenType::ACTION_MOUSE_SCROLL_UP;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "down")
                    {
                        type = TokenType::ACTION_MOUSE_SCROLL_DOWN;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "left")
                    {
                        type = TokenType::ACTION_MOUSE_SCROLL_LEFT;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "right")
                    {
                        type = TokenType::ACTION_MOUSE_SCROLL_RIGHT;
                        is_mouse_action = true;
                    }
                }
                if (mouse_kwd == "click")
                {
                    if (dir_kwd == "left")
                    {
                        type = TokenType::ACTION_MOUSE_CLICK_LEFT;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "right")
                    {
                        type = TokenType::ACTION_MOUSE_CLICK_RIGHT;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "center")
                    {
                        type = TokenType::ACTION_MOUSE_CLICK_CENTER;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "backwards" || dir_kwd == "back")
                    {
                        type = TokenType::ACTION_MOUSE_CLICK_BACKWARDS;
                        is_mouse_action = true;
                    }
                    if (dir_kwd == "forwards")
                    {
                        type = TokenType::ACTION_MOUSE_CLICK_FORWARDS;
                        is_mouse_action = true;
                    }
                }

                // if a mouse action wasn't found, restore index
                if (!is_mouse_action)
                    index = old_index;
            }

            if (identifier == "press")
                type = TokenType::ACTION_PRESS;
            if (identifier == "release")
                type = TokenType::ACTION_RELEASE;
            if (identifier == "click")
                type = TokenType::ACTION_CLICK;
            if (identifier == "wait")
                type = TokenType::ACTION_WAIT;
            if (identifier == "toggle")
                type = TokenType::ACTION_TOGGLE;
            if (identifier == "leave")
                type = TokenType::ACTION_LEAVE;
            if (identifier == "type")
                type = TokenType::ACTION_TYPE;
            if (identifier == "bootloader")
                type = TokenType::ACTION_BOOTLOADER;
            if (identifier == "home")
                type = TokenType::ACTION_HOME;
            if (identifier == "nothing")
                type = TokenType::ACTION_NOTHING;
            if (identifier == "quickly")
                type = TokenType::PARAMETER_QUICKLY;
            if (identifier == "slowly")
                type = TokenType::PARAMETER_SLOWLY;
            if (identifier == "repeatedly")
                type = TokenType::PARAMETER_REPEATEDLY;
            if (identifier == "ms" || identifier == "millisecond" || identifier == "milliseconds")
                type = TokenType::PARAMETER_TIME_MS;
            if (identifier == "sec" || identifier == "second" || identifier == "seconds")
                type = TokenType::PARAMETER_TIME_SEC;
            if (identifier == "min" || identifier == "minute" || identifier == "minutes")
                type = TokenType::PARAMETER_TIME_MIN;

            if (identifier == "switch" && to_lower(source.substr(index, 3)) == " to")
            {
                index += 3;
                type = TokenType::ACTION_SWITCH_TO;
            }
            if (identifier == "reset" && to_lower(source.substr(index, 9)) == " keyboard")
            {
                index += 9;
                type = TokenType::ACTION_RESET_KEYBOARD;
            }
            if (identifier == "pass" && to_lower(source.substr(index, 8)) == " through")
            {
                index += 8;
                type = TokenType::ACTION_PASS_THROUGH;
            }
            if (identifier == "reload" && to_lower(source.substr(index, 9)) == " key maps")
            {
                index += 9;
                type = TokenType::ACTION_RELOAD_KEY_MAPS;
            }
            if (identifier == "at" && to_lower(source.substr(index, 1)) == " human speed")
            {
                index += 12;
                type = TokenType::PARAMETER_AT_HUMAN_SPEED;
            }
            if (identifier == "until" && to_lower(source.substr(index, 9)) == " released")
            {
                index += 9;
                type = TokenType::PARAMETER_UNTIL_RELEASED;
            }
            if (identifier == "other" && to_lower(source.substr(index, 18)) == " keys fall through")
            {
                index += 18;
                type = TokenType::TOP_OTHER_KEYS_FALL_THROUGH;
            }
            if (identifier == "block" && to_lower(source.substr(index, 11)) == " other keys")
            {
                index += 11;
                type = TokenType::TOP_BLOCK_OTHER_KEYS;
            }

            tokens.push_back({
                .type = type,
                .start = start,
                .length = index - start,
                .line_number = line_number,
            });
        }
        return {"", tokens};
    }

}