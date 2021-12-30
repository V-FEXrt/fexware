#include "parser.h"

#include <algorithm>
#include <iostream>
#include <optional>
#include <unordered_map>

#include "actions.h"
#include "layer.h"
#include "tokenizer.h"

#define ROWKEY_VALUE(row, key) ((row * 12) + key)

namespace fex
{

	const std::unordered_map<std::string, int> key_names({{"A", 0x04}, // Keyboard a and A
														  {"B", 0x05}, // Keyboard b and B
														  {"C", 0x06}, // Keyboard c and C
														  {"D", 0x07}, // Keyboard d and D
														  {"E", 0x08}, // Keyboard e and E
														  {"F", 0x09}, // Keyboard f and F
														  {"G", 0x0a}, // Keyboard g and G
														  {"H", 0x0b}, // Keyboard h and H
														  {"I", 0x0c}, // Keyboard i and I
														  {"J", 0x0d}, // Keyboard j and J
														  {"K", 0x0e}, // Keyboard k and K
														  {"L", 0x0f}, // Keyboard l and L
														  {"M", 0x10}, // Keyboard m and M
														  {"N", 0x11}, // Keyboard n and N
														  {"O", 0x12}, // Keyboard o and O
														  {"P", 0x13}, // Keyboard p and P
														  {"Q", 0x14}, // Keyboard q and Q
														  {"R", 0x15}, // Keyboard r and R
														  {"S", 0x16}, // Keyboard s and S
														  {"T", 0x17}, // Keyboard t and T
														  {"U", 0x18}, // Keyboard u and U
														  {"V", 0x19}, // Keyboard v and V
														  {"W", 0x1a}, // Keyboard w and W
														  {"X", 0x1b}, // Keyboard x and X
														  {"Y", 0x1c}, // Keyboard y and Y
														  {"Z", 0x1d}, // Keyboard z and Z

														  {"1", 0x1e}, // Keyboard 1 and !
														  {"2", 0x1f}, // Keyboard 2 and @
														  {"3", 0x20}, // Keyboard 3 and//
														  {"4", 0x21}, // Keyboard 4 and $
														  {"5", 0x22}, // Keyboard 5 and %
														  {"6", 0x23}, // Keyboard 6 and ^
														  {"7", 0x24}, // Keyboard 7 and &
														  {"8", 0x25}, // Keyboard 8 and *
														  {"9", 0x26}, // Keyboard 9 and (
														  {"0", 0x27}, // Keyboard 0 and )

														  {"ENTER", 0x28},		  // Keyboard Return (ENTER)
														  {"ESC", 0x29},		  // Keyboard ESCAPE
														  {"ESCAPE", 0x29},		  // Keyboard ESCAPE
														  {"BACKSPACE", 0x2a},	  // Keyboard DELETE (Backspace)
														  {"TAB", 0x2b},		  // Keyboard Tab
														  {"SPACE", 0x2c},		  // Keyboard Spacebar
														  {"LEFTBRACE", 0x2f},	  // Keyboard [ and {
														  {"LEFTBRACKET", 0x2f},  // Keyboard [ and {
														  {"MINUS", 0x2d},		  // Keyboard - and _
														  {"EQUAL", 0x2e},		  // Keyboard = and +
														  {"EQUALS", 0x2e},		  // Keyboard = and +
														  {"PLUS", 0x2e},		  // Keyboard = and +
														  {"RIGHTBRACE", 0x30},	  // Keyboard ] and }
														  {"RIGHTBRACKET", 0x30}, // Keyboard ] and }
														  {"BACKSLASH", 0x31},	  // Keyboard \ and |
														  {"HASHTILDE", 0x32},	  // Keyboard Non-US // and ~
														  {"HASHANDTILDE", 0x32}, // Keyboard Non-US // and ~
														  {"SEMICOLON", 0x33},	  // Keyboard ; and ,
														  {"APOSTROPHE", 0x34},	  // Keyboard " and "
														  {"QUOTE", 0x34},		  // Keyboard " and "
														  {"GRAVE", 0x35},		  // Keyboard ` and ~
														  {"BACKTICK", 0x35},	  // Keyboard ` and ~
														  {"TILDE", 0x35},		  // Keyboard ` and ~
														  {"COMMA", 0x36},		  // Keyboard , and <
														  {"DOT", 0x37},		  // Keyboard . and >
														  {"PERIOD", 0x37},		  // Keyboard . and >
														  {"SLASH", 0x38},		  // Keyboard / and ?
														  {"FORWARDSLASH", 0x38}, // Keyboard / and ?
														  {"CAPSLOCK", 0x39},	  // Keyboard Caps Lock

														  {"F1", 0x3a},	 // Keyboard F1
														  {"F2", 0x3b},	 // Keyboard F2
														  {"F3", 0x3c},	 // Keyboard F3
														  {"F4", 0x3d},	 // Keyboard F4
														  {"F5", 0x3e},	 // Keyboard F5
														  {"F6", 0x3f},	 // Keyboard F6
														  {"F7", 0x40},	 // Keyboard F7
														  {"F8", 0x41},	 // Keyboard F8
														  {"F9", 0x42},	 // Keyboard F9
														  {"F10", 0x43}, // Keyboard F10
														  {"F11", 0x44}, // Keyboard F11
														  {"F12", 0x45}, // Keyboard F12

														  {"SYSRQ", 0x46},		// Keyboard Print Screen
														  {"SCROLLLOCK", 0x47}, // Keyboard Scroll Lock
														  {"PAUSE", 0x48},		// Keyboard Pause
														  {"INSERT", 0x49},		// Keyboard Insert
														  {"HOME", 0x4a},		// Keyboard Home
														  {"PAGEUP", 0x4b},		// Keyboard Page Up
														  {"DELETE", 0x4c},		// Keyboard Delete Forward
														  {"END", 0x4d},		// Keyboard End
														  {"PAGEDOWN", 0x4e},	// Keyboard Page Down
														  {"RIGHT", 0x4f},		// Keyboard Right Arrow
														  {"RIGHTARROW", 0x4f}, // Keyboard Right Arrow
														  {"LEFT", 0x50},		// Keyboard Left Arrow
														  {"LEFTARROW", 0x50},	// Keyboard Left Arrow
														  {"DOWN", 0x51},		// Keyboard Down Arrow
														  {"DOWNARROW", 0x51},	// Keyboard Down Arrow
														  {"UP", 0x52},			// Keyboard Up Arrow
														  {"UPARROW", 0x52},	// Keyboard Up Arrow

														  {"NUMLOCK", 0x53},		// Keyboard Num Lock and Clear
														  {"KPSLASH", 0x54},		// Keypad /
														  {"NUMPADSLASH", 0x54},	// Keypad /
														  {"KPASTERISK", 0x55},		// Keypad *
														  {"NUMPADASTERISK", 0x55}, // Keypad *
														  {"NUMPADTIMES", 0x55},	// Keypad *
														  {"KPMINUS", 0x56},		// Keypad -
														  {"NUMPADMINUS", 0x56},	// Keypad -
														  {"KPPLUS", 0x57},			// Keypad +
														  {"NUMPADPLUS", 0x57},		// Keypad +
														  {"KPENTER", 0x58},		// Keypad ENTER
														  {"NUMPADENTER", 0x58},	// Keypad ENTER
														  {"KP1", 0x59},			// Keypad 1 and End
														  {"NUMPAD1", 0x59},		// Keypad 1 and End
														  {"KP2", 0x5a},			// Keypad 2 and Down Arrow
														  {"NUMPAD2", 0x5a},		// Keypad 2 and Down Arrow
														  {"KP3", 0x5b},			// Keypad 3 and PageDn
														  {"NUMPAD3", 0x5b},		// Keypad 3 and PageDn
														  {"KP4", 0x5c},			// Keypad 4 and Left Arrow
														  {"NUMPAD4", 0x5c},		// Keypad 4 and Left Arrow
														  {"KP5", 0x5d},			// Keypad 5
														  {"NUMPAD5", 0x5d},		// Keypad 5
														  {"KP6", 0x5e},			// Keypad 6 and Right Arrow
														  {"NUMPAD6", 0x5e},		// Keypad 6 and Right Arrow
														  {"KP7", 0x5f},			// Keypad 7 and Home
														  {"NUMPAD7", 0x5f},		// Keypad 7 and Home
														  {"KP8", 0x60},			// Keypad 8 and Up Arrow
														  {"NUMPAD8", 0x60},		// Keypad 8 and Up Arrow
														  {"KP9", 0x61},			// Keypad 9 and Page Up
														  {"NUMPAD9", 0x61},		// Keypad 9 and Page Up
														  {"KP0", 0x62},			// Keypad 0 and Insert
														  {"NUMPAD0", 0x62},		// Keypad 0 and Insert
														  {"KPDOT", 0x63},			// Keypad . and Delete
														  {"NUMPADDOT", 0x63},		// Keypad . and Delete

														  {"102ND", 0x64},		  // Keyboard Non-US \ and |
														  {"COMPOSE", 0x65},	  // Keyboard Application
														  {"POWER", 0x66},		  // Keyboard Power
														  {"KPEQUAL", 0x67},	  // Keypad =
														  {"NUMPADEQUAL", 0x67},  // Keypad =
														  {"NUMPADEQUALS", 0x67}, // Keypad =

														  {"F13", 0x68}, // Keyboard F13
														  {"F14", 0x69}, // Keyboard F14
														  {"F15", 0x6a}, // Keyboard F15
														  {"F16", 0x6b}, // Keyboard F16
														  {"F17", 0x6c}, // Keyboard F17
														  {"F18", 0x6d}, // Keyboard F18
														  {"F19", 0x6e}, // Keyboard F19
														  {"F20", 0x6f}, // Keyboard F20
														  {"F21", 0x70}, // Keyboard F21
														  {"F22", 0x71}, // Keyboard F22
														  {"F23", 0x72}, // Keyboard F23
														  {"F24", 0x73}, // Keyboard F24

														  {"OPEN", 0x74},			  // Keyboard Execute
														  {"HELP", 0x75},			  // Keyboard Help
														  {"PROPS", 0x76},			  // Keyboard Menu
														  {"FRONT", 0x77},			  // Keyboard Select
														  {"STOP", 0x78},			  // Keyboard Stop
														  {"AGAIN", 0x79},			  // Keyboard Again
														  {"UNDO", 0x7a},			  // Keyboard Undo
														  {"CUT", 0x7b},			  // Keyboard Cut
														  {"COPY", 0x7c},			  // Keyboard Copy
														  {"PASTE", 0x7d},			  // Keyboard Paste
														  {"FIND", 0x7e},			  // Keyboard Find
														  {"MUTE", 0x7f},			  // Keyboard Mute
														  {"VOLUMEUP", 0x80},		  // Keyboard Volume Up
														  {"VOLUMEDOWN", 0x81},		  // Keyboard Volume Down
														  {"KPCOMMA", 0x85},		  // Keypad Comma
														  {"KEYPAD EQUAL", 0x86},	  // Keypad Equal Sign
														  {"RO", 0x87},				  // Keyboard International1
														  {"KATAKANAHIRAGANA", 0x88}, // Keyboard International2
														  {"YEN", 0x89},			  // Keyboard International3
														  {"HENKAN", 0x8a},			  // Keyboard International4
														  {"MUHENKAN", 0x8b},		  // Keyboard International5
														  {"KPJPCOMMA", 0x8c},		  // Keyboard International6
														  {"HANGEUL", 0x90},		  // Keyboard LANG1
														  {"HANJA", 0x91},			  // Keyboard LANG2
														  {"KATAKANA", 0x92},		  // Keyboard LANG3
														  {"HIRAGANA", 0x93},		  // Keyboard LANG4
														  {"ZENKAKUHANKAKU", 0x94},	  // Keyboard LANG5
														  {"KPLEFTPAREN", 0xb6},	  // Keypad (
														  {"KPRIGHTPAREN", 0xb7},	  // Keypad )

														  {"CTRL", 0xe0},		  // Keyboard Left Control
														  {"CONTROL", 0xe0},	  // Keyboard Left Control
														  {"LEFTCTRL", 0xe0},	  // Keyboard Left Control
														  {"LEFTCONTROL", 0xe0},  // Keyboard Left Control
														  {"SHIFT", 0xe1},		  // Keyboard Left Shift
														  {"LEFTSHIFT", 0xe1},	  // Keyboard Left Shift
														  {"ALT", 0xe2},		  // Keyboard Left Alt
														  {"LEFTALT", 0xe2},	  // Keyboard Left Alt
														  {"LEFTMETA", 0xe3},	  // Keyboard Left GUI
														  {"WINDOWS", 0xe3},	  // Keyboard Left GUI
														  {"GUI", 0xe3},		  // Keyboard Left GUI
														  {"LEFTGUI", 0xe3},	  // Keyboard Left GUI
														  {"LEFTWINDOWS", 0xe3},  // Keyboard Left GUI
														  {"RIGHTCTRL", 0xe4},	  // Keyboard Right Control
														  {"RIGHTCONTROL", 0xe4}, // Keyboard Right Control
														  {"RIGHTSHIFT", 0xe5},	  // Keyboard Right Shift
														  {"RIGHTALT", 0xe6},	  // Keyboard Right Alt
														  {"RIGHTMETA", 0xe7},	  // Keyboard Right GUI
														  {"RIGHTWINDOWS", 0xe7}, // Keyboard Right GUI
														  {"RIGHTGUI", 0xe7},	  // Keyboard Right GUI

														  {"MEDIAPLAYPAUSE", 0xe8},
														  {"MEDIASTOPCD", 0xe9},
														  {"MEDIAPREVIOUSSONG", 0xea},
														  {"MEDIANEXTSONG", 0xeb},
														  {"MEDIAEJECTCD", 0xec},
														  {"VOLUMEUP", 0xed},
														  {"MEDIAVOLUMEDOWN", 0xee},
														  {"MEDIAMUTE", 0xef},
														  {"MUTE", 0xef},
														  {"MEDIAWWW", 0xf0},
														  {"MEDIABACK", 0xf1},
														  {"BACK", 0xf1},
														  {"MEDIAFORWARD", 0xf2},
														  {"FORWARD", 0xf2},
														  {"MEDIASTOP", 0xf3},
														  {"MEDIAFIND", 0xf4},
														  {"MEDIASCROLLUP", 0xf5},
														  {"MEDIASCROLLDOWN", 0xf6},
														  {"MEDIAEDIT", 0xf7},
														  {"MEDIASLEEP", 0xf8},
														  {"MEDIACOFFEE", 0xf9},
														  {"MEDIAREFRESH", 0xfa},
														  {"MEDIACALC", 0xfb}});

	Operation operation_from_token(const TokenType &type)
	{
		switch (type)
		{
		case TokenType::OPERATION_CLICK:
			return Operation::CLICK;
		case TokenType::OPERATION_PRESS:
			return Operation::PRESS;
		case TokenType::OPERATION_HOLD:
			return Operation::HOLD;
		case TokenType::OPERATION_DOUBLE_CLICK:
			return Operation::DOUBLE_CLICK;
		case TokenType::OPERATION_RELEASE:
			return Operation::RELEASE;
		default:
			return Operation::RELEASE;
		}
	}

	std::pair<std::string, std::vector<Token>> parse_action(
		const std::string &source, const std::vector<Token> &tokens, int *index)
	{
		// Note: MUST BE KEPT IN SORTED ORDER
		std::vector<TokenType> front_disallowed_tokens = {
			TokenType::SYM_COMMA,
			TokenType::SYM_PLUS,
			TokenType::STRING_LIT,
			TokenType::PARAMETER_QUICKLY,
			TokenType::PARAMETER_SLOWLY,
			TokenType::PARAMETER_REPEATEDLY,
			TokenType::PARAMETER_AT_HUMAN_SPEED,
			TokenType::PARAMETER_UNTIL_RELEASED,
		};

		// Note: MUST BE KEPT IN SORTED ORDER
		std::vector<TokenType> back_disallowed_tokens = {
			TokenType::SYM_COMMA,
			TokenType::SYM_PLUS,
		};

		// Note: MUST BE KEPT IN SORTED ORDER
		std::vector<TokenType> allowed_tokens = {
			TokenType::SYM_COMMA,
			TokenType::SYM_PLUS,
			TokenType::STRING_LIT,
			TokenType::NUM_LIT,
			TokenType::HEX_LIT,
			TokenType::IDENTIFIER,
			TokenType::ACTION_PRESS,
			TokenType::ACTION_RELEASE,
			TokenType::ACTION_CLICK,
			TokenType::ACTION_WAIT,
			TokenType::ACTION_SWITCH_TO,
			TokenType::ACTION_TOGGLE,
			TokenType::ACTION_LEAVE,
			TokenType::ACTION_TYPE,
			TokenType::ACTION_RESET_KEYBOARD,
			TokenType::ACTION_BOOTLOADER,
			TokenType::ACTION_HOME,
			TokenType::ACTION_NOTHING,
			TokenType::ACTION_PASS_THROUGH,
			TokenType::ACTION_RELOAD_KEY_MAPS,
			TokenType::ACTION_MOUSE_MOVE_UP,
			TokenType::ACTION_MOUSE_MOVE_DOWN,
			TokenType::ACTION_MOUSE_MOVE_LEFT,
			TokenType::ACTION_MOUSE_MOVE_RIGHT,
			TokenType::ACTION_MOUSE_SCROLL_UP,
			TokenType::ACTION_MOUSE_SCROLL_DOWN,
			TokenType::ACTION_MOUSE_SCROLL_LEFT,
			TokenType::ACTION_MOUSE_SCROLL_RIGHT,
			TokenType::ACTION_MOUSE_CLICK_LEFT,
			TokenType::ACTION_MOUSE_CLICK_RIGHT,
			TokenType::ACTION_MOUSE_CLICK_CENTER,
			TokenType::ACTION_MOUSE_CLICK_BACKWARDS,
			TokenType::ACTION_MOUSE_CLICK_FORWARDS,
			TokenType::PARAMETER_QUICKLY,
			TokenType::PARAMETER_SLOWLY,
			TokenType::PARAMETER_REPEATEDLY,
			TokenType::PARAMETER_AT_HUMAN_SPEED,
			TokenType::PARAMETER_UNTIL_RELEASED,
			TokenType::PARAMETER_TIME_MS,
			TokenType::PARAMETER_TIME_SEC,
			TokenType::PARAMETER_TIME_MIN,
		};

		const Token &head = tokens[*index];
		if (std::binary_search(front_disallowed_tokens.begin(), front_disallowed_tokens.end(), head.type))
		{
			return {errmsg("Token not allowed at start of action: " + TokenStr(source, head), head.line_number), {}};
		}

		(*index)++;
		int start = *index;
		std::vector<Token> action = {head};

		while (*index < tokens.size() && std::binary_search(allowed_tokens.begin(), allowed_tokens.end(), tokens[*index].type))
		{
			action.push_back(tokens[*index]);
			(*index)++;
		}

		// Check for trailing two '+' and ','
		if (std::binary_search(back_disallowed_tokens.begin(), back_disallowed_tokens.end(), action.back().type))
		{
			return {errmsg("Token not allowed at end of action: " + TokenStr(source, action.back()), action.back().line_number), {}};
		}

		// Check for two '+' in a row
		for (int i = 0; i < action.size() - 1; i++)
		{
			if (action[i].type == TokenType::SYM_PLUS && action[i + 1].type == TokenType::SYM_PLUS)
			{
				return {errmsg("Cannot have two consecutive '+'", action[i].line_number), {}};
			}
		}

		return {"", std::move(action)};
	}

	std::pair<std::string, std::pair<TopLevel, BindingList>> parse(const std::string &source, const std::vector<Token> &tokens)
	{
		TopLevel top_level;
		BindingList bindings;

		int index = 0;
		while (index < tokens.size())
		{
			const Token &row = tokens[index];

			if (row.type == TokenType::TOP_BLOCK_OTHER_KEYS || row.type == TokenType::TOP_OTHER_KEYS_FALL_THROUGH)
			{
				top_level.push_back(row);
				index++;
				continue;
			}

			if (row.type != TokenType::ROW_LIT)
			{
				return {errmsg("Expected row literal, saw: " + TokenStr(source, row), row.line_number), {}};
			}

			index++;

			if (index >= tokens.size() || tokens[index].type != TokenType::SYM_COMMA)
			{
				return {errmsg("Expected comma after: " + TokenStr(source, row), row.line_number), {}};
			}

			index++;

			if (index >= tokens.size() || tokens[index].type != TokenType::KEY_LIT)
			{
				return {errmsg("Expected key literal", tokens[index].line_number), {}};
			}

			const Token &key = tokens[index];
			index++;

			if (index >= tokens.size() || tokens[index].type != TokenType::SYM_COLON)
			{
				return {errmsg("Expected colon after: " + TokenStr(source, key), key.line_number), {}};
			}

			index++;
			int row_val = std::atoi(TokenStr(source, row).erase(0, 1).c_str());
			int key_val = std::atoi(TokenStr(source, key).erase(0, 1).c_str());
			std::cout << row_val << " " << key_val << std::endl;

			Binding binding;
			binding.insert({ROWKEY_VALUE(row_val, key_val), {}});

			bool is_inline = true;

			// Note: MUST BE KEPT IN SORTERD ORDER
			std::vector<TokenType> on_x_tokens = {
				TokenType::OPERATION_PRESS,
				TokenType::OPERATION_CLICK,
				TokenType::OPERATION_HOLD,
				TokenType::OPERATION_DOUBLE_CLICK,
				TokenType::OPERATION_RELEASE};

			// if there are any on-x events, process them all
			while (index < tokens.size() && std::binary_search(on_x_tokens.begin(), on_x_tokens.end(), tokens[index].type))
			{
				is_inline = false;

				const Token &operation = tokens[index];
				index++;

				if (index >= tokens.size() || tokens[index].type != TokenType::SYM_COLON)
				{
					return {errmsg("Expected colon after: " + TokenStr(source, operation), operation.line_number), {}};
				}
				index++;

				if (index >= tokens.size())
				{
					return {errmsg("Expected action definition after: " + TokenStr(source, operation), operation.line_number), {}};
				}

				auto operation_actions = parse_action(source, tokens, &index);
				if (operation_actions.first != "")
				{
					return {operation_actions.first, {}};
				}

				binding[ROWKEY_VALUE(row_val, key_val)][operation_from_token(operation.type)] = std::move(operation_actions.second);
			}

			// otherwise process the inline statement
			if (is_inline)
			{
				if (index >= tokens.size())
				{
					return {errmsg("Expected action definition after: " + TokenStr(source, key), key.line_number), {}};
				}

				int old_index = index;
				auto actions = parse_action(source, tokens, &index);
				if (actions.first != "")
				{
					return {actions.first, {}};
				}

				// If we aren't told explictly, bind to press
				// TODO(fex): bind also to RELEASE?
				binding[ROWKEY_VALUE(row_val, key_val)][Operation::PRESS] = std::move(actions.second);
			}

			bindings.push_back(std::move(binding));
		}

		return {"", {std::move(top_level), std::move(bindings)}};
	}

	std::pair<std::string, unsigned long> parse_time(const std::string &source, const std::vector<Token> &tokens)
	{
		if (tokens.size() != 2)
		{
			return {errmsg("Expected 2 time parameters, saw: " + std::to_string(tokens.size()), tokens[0].line_number), {}};
		}

		const Token &duration = tokens[0];
		const Token &units = tokens[1];

		if (duration.type != TokenType::NUM_LIT)
		{
			return {errmsg("Expected number in time literal", duration.line_number), {}};
		}

		unsigned long time = std::stoul(TokenStr(source, duration));

		if (units.type == TokenType::PARAMETER_TIME_MS)
		{
			return {"", time};
		}

		if (units.type == TokenType::PARAMETER_TIME_SEC)
		{
			return {"", time * 1000};
		}

		if (units.type == TokenType::PARAMETER_TIME_MIN)
		{
			return {"", time * 1000 * 60};
		}

		return {errmsg("Expected units in time literal", units.line_number), {}};
	}

	std::pair<std::string, std::vector<int>> parse_key_codes(const std::string &source, const std::vector<Token> &tokens)
	{
		std::vector<int> key_codes;
		std::vector<std::vector<Token>> subtokens;
		subtokens.push_back({});

		// Split token list [LEFT, ALT, +, F, +, A]
		// into [[LEFT, ALT], [F], [A]]
		for (const Token &token : tokens)
		{
			if (token.type == TokenType::SYM_PLUS)
			{
				subtokens.push_back({});
				continue;
			}
			subtokens.back().push_back(token);
		}

		for (const std::vector<Token> &token : subtokens)
		{
			// Process hex literal keys
			if (token[0].type == TokenType::HEX_LIT)
			{
				if (token.size() != 1)
				{
					return {errmsg("Hex literals must be separated by '+'", token[0].line_number), {}};
				}

				key_codes.push_back(strtoul(TokenStr(source, token[0]).c_str(), NULL, 16));
				continue;
			}

			// Merge tokens into single name [LEFT, ALT] => 'LEFT ALT'
			std::string key_name = "";
			for (const Token &t : token)
			{
				key_name = key_name + TokenStr(source, t);
			}

			printf("val: %s\n", key_name.c_str());

			auto item = key_names.find(key_name);
			if (item == key_names.end())
			{
				return {errmsg("Invalid Action or Key: '" + TokenStr(source, token[0]) + "'", token[0].line_number), {}};
			}

			key_codes.push_back(item->second);
		}

		return {"", std::move(key_codes)};
	}

	std::pair<std::string, std::unique_ptr<BoundAction>> parse_action_token(const std::string &source, const std::vector<Token> &tokens, Operation operation)
	{
		const Token &action_token = tokens[0];
		const std::vector<Token> rest = std::vector<Token>{tokens.begin() + 1, tokens.end()};

		switch (action_token.type)
		{
		case TokenType::ACTION_PRESS:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Press action requires key parameter", action_token.line_number), nullptr};
			}

			auto key_codes = parse_key_codes(source, rest);
			if (key_codes.first != "")
			{
				return {key_codes.first, nullptr};
			}
			return {"", std::make_unique<PressKeyAction>(key_codes.second)};
		}

		case TokenType::ACTION_RELEASE:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Release action requires key parameter", action_token.line_number), nullptr};
			}

			auto key_codes = parse_key_codes(source, rest);
			if (key_codes.first != "")
			{
				return {key_codes.first, nullptr};
			}
			return {"", std::make_unique<ReleaseKeyAction>(key_codes.second)};
		}
		case TokenType::ACTION_CLICK:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Click action requires key parameter", action_token.line_number), nullptr};
			}

			auto key_codes = parse_key_codes(source, rest);
			if (key_codes.first != "")
			{
				return {key_codes.first, nullptr};
			}
			return {"", std::make_unique<ClickKeyAction>(key_codes.second)};
		}
		case TokenType::ACTION_WAIT:
		{
			if (tokens.size() != 3)
			{
				return {errmsg("Wait action requires 2 parameters", action_token.line_number), nullptr};
			}
			auto time = parse_time(source, rest);
			if (time.first != "")
			{
				return {time.first, nullptr};
			}
			return {"", std::make_unique<DelayAction>(time.second)};
		}
		case TokenType::ACTION_SWITCH_TO:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Switch to action requires layer parameter", action_token.line_number), nullptr};
			}

			if (tokens.size() == 2 && tokens[1].type == TokenType::PARAMETER_UNTIL_RELEASED)
			{
				return {errmsg("Missing layer name for temporary switch: '" + TokenRunStr(source, tokens[0], tokens.back()) + "'", action_token.line_number), nullptr};
			}

			if (tokens.back().type == TokenType::PARAMETER_UNTIL_RELEASED)
			{
				if (operation != Operation::HOLD)
				{
					return {errmsg("TemporaryLayerAction can only bind to On Hold", action_token.line_number), nullptr};
				}

				std::string layer_name = TokenRunStr(source, tokens[1], tokens[tokens.size() - 2]);
				return {"", std::make_unique<TemporaryLayerAction>(layer_name)};
			}

			std::string layer_name = TokenRunStr(source, tokens[1], tokens.back());
			return {"", std::make_unique<SwitchToLayerAction>(layer_name)};
		}
		case TokenType::ACTION_TOGGLE:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Toggle action requires layer parameter", action_token.line_number), nullptr};
			}

			std::string param = TokenRunStr(source, tokens[1], tokens.back());
			return {"", std::make_unique<ToggleLayerAction>(param)};
		}
		case TokenType::ACTION_LEAVE:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Leave action requires layer parameter", action_token.line_number), nullptr};
			}

			std::string param = TokenRunStr(source, tokens[1], tokens.back());
			return {"", std::make_unique<LeaveLayerAction>(param)};
		}
		case TokenType::ACTION_RESET_KEYBOARD:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Reset Keyboard action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<ResetKeebAction>()};
		}
		case TokenType::ACTION_BOOTLOADER:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Bootloader action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<KeebBootloaderAction>()};
		}
		case TokenType::ACTION_HOME:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Home action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<ResetLayerAction>()};
		}
		case TokenType::ACTION_NOTHING:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Nothing action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<NothingburgerAction>()};
		}
		case TokenType::ACTION_PASS_THROUGH:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Pass through action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<PassThroughAction>()};
		}
		case TokenType::ACTION_RELOAD_KEY_MAPS:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Reload Key Maps action shouldn\'t have any parameters", action_token.line_number), nullptr};
			}

			return {"", std::make_unique<ReloadKeymapAction>()};
		}
		case TokenType::ACTION_TYPE:
		{
			if (tokens.size() == 1)
			{
				return {errmsg("Type action missing text parameter", action_token.line_number), nullptr};
			}

			auto string_lit = tokens[1];
			if (string_lit.type != TokenType::STRING_LIT)
			{
				return {errmsg("Type action's first parameter must be quoted text", action_token.line_number), nullptr};
			}

			unsigned long delay = 10; // in milliseconds

			// Adjust both sides by 1 to remove quotes
			std::string str = TokenStr(source, string_lit);
			std::string string_to_type = str.substr(1, str.size() - 2);

			bool repeating = false;
			int time_keyword_count = 0;

			std::unordered_map<std::string, std::string> replacements = {
				// {"[COMMA]", ","}, // Not needed, just type ,
				{"[DOUBLE QUOTES]", "\""},
				{"[SINGLE QUOTE]", "'"},
				{"[RETURN]", "\n"},
				// TODO: Should escape sequences be allowed? i.e. '\n' in the text
			};

			for (auto &it : replacements)
			{
				int index = string_to_type.find(it.first);
				if (index != std::string::npos)
				{
					string_to_type = string_to_type.replace(index, it.first.size(), it.second);
				}
			}

			std::vector<Token> time_tokens;

			for (const Token &token : std::vector<Token>{tokens.begin() + 2, tokens.end()})
			{
				switch (token.type)
				{
				case TokenType::PARAMETER_REPEATEDLY: // TODO: 'once' and timing strings are mutually exclusive, improve parsing to catch multiple tokens
					repeating = true;
					break;
				case TokenType::PARAMETER_SLOWLY:
					delay = 200;
					time_keyword_count++;
					break;
				case TokenType::PARAMETER_QUICKLY:
					delay = 0;
					time_keyword_count++;
					break;
				case TokenType::PARAMETER_AT_HUMAN_SPEED:
					delay = 50;
					time_keyword_count++;
					break;
				case TokenType::NUM_LIT:
				case TokenType::PARAMETER_TIME_MS:
				case TokenType::PARAMETER_TIME_SEC:
				case TokenType::PARAMETER_TIME_MIN:
					time_tokens.push_back(token);
					break;

				default:
					break;
				}
			}

			if (time_tokens.size() != 0 && time_tokens.size() != 2)
			{
				return {errmsg("Incorrect number of time tokens provided", action_token.line_number), nullptr};
			}

			if (time_tokens.size() == 2)
			{
				auto parsed = parse_time(source, time_tokens);
				if (parsed.first != "")
				{
					return {parsed.first, nullptr};
				}
				delay = parsed.second;
				time_keyword_count++;
			}

			if (time_keyword_count > 1)
			{
				return {errmsg("Multiple speeds set for Type action. Please select one.\n\t" + TokenRunStr(source, tokens[0], tokens[tokens.size() - 1]), action_token.line_number), nullptr};
			}

			if (repeating)
			{
				return {"", std::make_unique<StringTyperAction>(string_to_type, delay, 0)};
			}

			return {"", std::make_unique<NonRepeatingStringTyperAction>(string_to_type, delay)};
		}

		case TokenType::ACTION_MOUSE_MOVE_UP:
		case TokenType::ACTION_MOUSE_MOVE_DOWN:
		case TokenType::ACTION_MOUSE_MOVE_LEFT:
		case TokenType::ACTION_MOUSE_MOVE_RIGHT:
		{
			if (tokens.size() != 2)
			{
				return {errmsg("Mouse move action requires distance parameter (0-100)", action_token.line_number), nullptr};
			}

			const Token &speed = tokens[1];
			if (speed.type != TokenType::NUM_LIT)
			{
				return {errmsg("Expected speed for mouse move", speed.line_number), nullptr};
			}

			long sp = std::stol(TokenStr(source, speed));
			if (sp < 0 || sp > 100) 
			{
				return {errmsg("Spped must be in range 0-100", speed.line_number), nullptr};
			}

			bool up_down = (action_token.type == TokenType::ACTION_MOUSE_MOVE_UP || action_token.type == TokenType::ACTION_MOUSE_MOVE_DOWN);

			int8_t pos_neg = 1;
			if (action_token.type == TokenType::ACTION_MOUSE_MOVE_UP || action_token.type == TokenType::ACTION_MOUSE_MOVE_LEFT)
			{
				pos_neg = -1;
			}

			return {"", std::make_unique<MouseMoveAction>(up_down, sp * pos_neg)};
		}

		case TokenType::ACTION_MOUSE_SCROLL_UP:
		case TokenType::ACTION_MOUSE_SCROLL_DOWN:
		case TokenType::ACTION_MOUSE_SCROLL_LEFT:
		case TokenType::ACTION_MOUSE_SCROLL_RIGHT:
		{
			if (tokens.size() != 2)
			{
				return {errmsg("Mouse scroll action requires distance parameter (0-100)", action_token.line_number), nullptr};
			}

			const Token &speed = tokens[1];
			if (speed.type != TokenType::NUM_LIT)
			{
				return {errmsg("Expected speed for mouse scroll", speed.line_number), nullptr};
			}

			long sp = std::stol(TokenStr(source, speed));
			if (sp < 0 || sp > 100) 
			{
				return {errmsg("Spped must be in range 0-100", speed.line_number), nullptr};
			}

			bool up_down = (action_token.type == TokenType::ACTION_MOUSE_SCROLL_UP || action_token.type == TokenType::ACTION_MOUSE_SCROLL_DOWN);

			// TODO(fex): double check this
			int8_t pos_neg = 1;
			if (action_token.type == TokenType::ACTION_MOUSE_SCROLL_DOWN || action_token.type == TokenType::ACTION_MOUSE_SCROLL_LEFT)
			{
				pos_neg = -1;
			}

			return {"", std::make_unique<MouseScrollAction>(up_down, sp * pos_neg)};
		}

		case TokenType::ACTION_MOUSE_CLICK_LEFT:
		case TokenType::ACTION_MOUSE_CLICK_RIGHT:
		case TokenType::ACTION_MOUSE_CLICK_CENTER:
		case TokenType::ACTION_MOUSE_CLICK_BACKWARDS:
		case TokenType::ACTION_MOUSE_CLICK_FORWARDS:
		{
			if (tokens.size() != 1)
			{
				return {errmsg("Mouse click should have 1 parameter", action_token.line_number), nullptr};
			}

            uint8_t button = 0;
			if (action_token.type == TokenType::ACTION_MOUSE_CLICK_LEFT) button = 1;
			if (action_token.type == TokenType::ACTION_MOUSE_CLICK_RIGHT) button = 2;
			if (action_token.type == TokenType::ACTION_MOUSE_CLICK_CENTER) button = 4;
			if (action_token.type == TokenType::ACTION_MOUSE_CLICK_BACKWARDS) button = 8;
			if (action_token.type == TokenType::ACTION_MOUSE_CLICK_FORWARDS) button = 16;


			return {"", std::make_unique<MouseClickAction>(button)};
		}

		}

		auto key_codes = parse_key_codes(source, tokens);
		if (key_codes.first != "")
		{
			return {key_codes.first, nullptr};
		}
		return {"", std::make_unique<GenericKeyAction>(key_codes.second)};
	}

	std::pair<std::string, std::unique_ptr<BoundAction>> parse_action_list(const std::string &source, const std::vector<Token> &tokens, Operation operation)
	{
		std::vector<std::vector<Token>> token_sets = {{}};

		// Split the token list on commas, each index into token_sets
		// represents a single action with any params
		for (const Token &token : tokens)
		{
			if (token.type == TokenType::SYM_COMMA)
			{
				token_sets.push_back({});
			}
			else
			{
				token_sets.back().push_back(token);
			}
		}

		std::vector<std::unique_ptr<BoundAction>> actions;
		for (const std::vector<Token> &token_set : token_sets)
		{
			auto parsed = parse_action_token(source, token_set, operation);
			if (parsed.first != "" || !parsed.second)
			{
				return {parsed.first, nullptr};
			}

			actions.push_back(std::move(parsed.second));
		}

		if (actions.size() == 1)
		{
			return {"", std::move(actions[0])};
		}

		return {"", std::make_unique<SequenceAction>(std::move(actions))};
	}

	std::string parse_source(const std::string &source, Layer *layer)
	{
		auto tokens = tokenize(source);
		if (tokens.first != "")
		{
			return tokens.first;
		}

		auto parsed = parse(source, tokens.second);
		if (parsed.first != "")
		{
			return parsed.first;
		}

		for (const Token &statement : parsed.second.first)
		{
			if (statement.type == TokenType::TOP_OTHER_KEYS_FALL_THROUGH)
			{
				layer->set_unassigned_keys_fall_through(true);
				continue;
			}
			if (statement.type == TokenType::TOP_BLOCK_OTHER_KEYS)
			{
				layer->set_unassigned_keys_fall_through(false);
				continue;
			}
		}

		for (const Binding &binding : parsed.second.second)
		{
			for (const auto &key : binding)
			{
				int key_val = key.first;
				const auto &operations = key.second;
				for (const auto &operation : operations)
				{
					Operation operation_val = operation.first;
					const std::vector<Token> &action_tokens = operation.second;

					printf("parsing action for key: %d\n", key_val);
					auto action = parse_action_list(source, action_tokens, operation_val);

					if (action.first != "" || !action.second)
					{
						return action.first;
					}

					layer->Bind(key_val, std::move(action.second), operation_val);
				}
			}
		}
		return "";
	}
}